/*
    Aayush Bhattarai
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
 */
#include "common.h"
#include "FIFOreqchannel.h"
#include <getopt.h>
#include <fstream>
#include <ctime>
#include <sys/wait.h>
#include "MQreqchannel.h"
#include "SHMreqchannel.h"
using namespace std;

class Person{
    public:
    int p;
    double t;
    int e;

    Person(){
        p = 0;
        t = 0;
        e = 0;
    }
};

void requesting_file(RequestChannel* channel, int capacity, 
                    string filename){   
    clock_t start, end;               
    //creating a binary file on recieved as the same name that was passed
    fstream file("./received/"+filename, ios::binary|ios::out);
    if(!file){ 
        cout << "Not opened" << endl;
    }
    int windowSize = MAX_MESSAGE;
    if(capacity > 0){
        windowSize = capacity;
    }
    filemsg fm(0, 0);
    char buf[sizeof (filemsg) + filename.size()+1];
    memcpy (buf, &fm, sizeof (filemsg));
    strcpy (buf + sizeof (filemsg), filename.c_str());
    // memcpy (buf+sizeof(filemsg),fname.c_str(), fname.size()+1);
    channel->cwrite(buf,sizeof (filemsg) + filename.size()+1);
    __int64_t fileLength;
    channel->cread(&fileLength, sizeof(__int64_t));
    int i = 0;
    int totalFileRead = 0;
    while(i < fileLength){
        if(totalFileRead+windowSize > fileLength-1){
            windowSize = fileLength-totalFileRead;
        }
        char recvbuf[windowSize];
        filemsg fm1(i, windowSize);
        memcpy(recvbuf, &fm1, sizeof(filemsg));
        memcpy(recvbuf + sizeof(filemsg), filename.c_str(), filename.size() + 1);
        channel->cwrite(&recvbuf, sizeof(recvbuf));
        channel->cread(&recvbuf, sizeof(recvbuf)+1);
        if(file.is_open()){
            file.write(recvbuf, windowSize);
        }
        i = i + windowSize;
        totalFileRead += windowSize;
    }
    file.close();
    end = clock() - start;
    cout << "Time Taken for copying " <<(double)end << "microseconds" << endl;
    
}

int main(int argc, char *argv[]){
    Person person_1;
    int opt;
    int capacity = MAX_MESSAGE;
    string fileName = "";
    bool newChannel = false;
    string ipcMethod = "f";
    int numChannels = 1;
    char* argument = (char *)to_string(capacity).c_str() ;
    while((opt = getopt(argc, argv, "p:t:e:m:f:c:i:"))!= -1){
        switch (opt){
            case 'p':
                person_1.p = atoi(optarg);
                break;
            case 't':
                person_1.t = atof(optarg);
                break;
            case 'e':
                person_1.e = atoi(optarg);
                break;
            case 'm':
                capacity = atoi(optarg);
                argument = (char *) to_string(capacity).c_str() ;
                break;
            case 'f':
                fileName = optarg;
                break;
            case 'c':
                newChannel = true;
                numChannels = atoi(optarg);
                break;
            case 'i':
                ipcMethod = optarg; 
                break;
        }   
    }

    if(fork() == 0){
        char* args[] = {"./server", "-m", argument , "-i", (char *)ipcMethod.c_str(),  NULL};
        if(execvp(args[0], args) < 0){
            perror("Server failed");
            exit(0);
        }
    }
    else{
        RequestChannel* control_channel;
        if(ipcMethod == "f"){
            control_channel = new FIFORequestChannel("control", RequestChannel::CLIENT_SIDE);
        }
        else if(ipcMethod == "q"){
            control_channel = new MQRequestChannel("control", RequestChannel::CLIENT_SIDE);
        }
        else if(ipcMethod == "s"){
            control_channel = new SHMRequestChannel("control", RequestChannel::CLIENT_SIDE, capacity);
        }
        
        if(person_1.e > 0 && person_1.t > 0 && person_1.p > 0 ){
            char buf [MAX_MESSAGE]; //creating a buffer of size MAX_MSG
            datamsg* x = new datamsg (person_1.p, person_1.t, person_1.e); //creating the msg on the heap
            control_channel->cwrite (x, sizeof (datamsg));   //sending the request to the 
            int nbytes = control_channel->cread (buf, MAX_MESSAGE);
            double reply = *(double *) buf;
            cout << "For person " << person_1.p<< ", at time  " << person_1.t <<", the value of ecg " << person_1.e << " is " << reply << endl;
            delete x;
        }
        else if(person_1.p > 0 && person_1.e > 0){
            clock_t start, end;
            //creating and writing using ios::out
            start = clock();
            int i = 0;
            int time = 0;
            while(i < numChannels){
                char buffer[30];
                MESSAGE_TYPE * newMsg = new MESSAGE_TYPE(NEWCHANNEL_MSG);
                control_channel->cwrite(newMsg, sizeof(MESSAGE_TYPE));
                control_channel->cread(&buffer, sizeof(buffer));
                RequestChannel* newChan;
                if(ipcMethod == "f"){
                    newChan = new FIFORequestChannel (buffer, RequestChannel::CLIENT_SIDE);
                }
                else if(ipcMethod == "q"){
                    newChan = new MQRequestChannel(buffer, RequestChannel::CLIENT_SIDE);
                }
                else if(ipcMethod == "s"){
                    newChan = new SHMRequestChannel(buffer, RequestChannel::CLIENT_SIDE, capacity);   
                }
                int pid = fork();
                if(pid == 0){
                    time = time + 0.004;
                    char buffer[MAX_MESSAGE];
                    //creating the datamsg on the stack
                    datamsg newMsg(person_1.p, time, person_1.e);
                    //sending the request to the channel 
                    newChan->cwrite(&newMsg, sizeof(datamsg));
                    int response = newChan->cread (buffer, MAX_MESSAGE);
                    double reply1 = *(double *) buffer;
                    cout << reply1 << endl;
                }
                else{
                    wait(0);
                    MESSAGE_TYPE quitmsg = QUIT_MSG;
                    newChan->cwrite(&quitmsg, sizeof (MESSAGE_TYPE));
                    delete newMsg;  
                }
            }
            end = clock() - start;
            cout << "Time Taken for copying " <<(double)end << "microseconds" << endl;
        }
        if(fileName != ""){
            requesting_file(control_channel, capacity, fileName);
        }
        if(newChannel){
            cout << "creating new channel" << endl;
            char buffer[30];
            //creating a newchannel messege 
            MESSAGE_TYPE * newMsg = new MESSAGE_TYPE(NEWCHANNEL_MSG);
            control_channel->cwrite(newMsg, sizeof(MESSAGE_TYPE));
            control_channel->cread(&buffer, sizeof(buffer));

            RequestChannel* newChan;
            if(ipcMethod == "f"){
                newChan = new FIFORequestChannel (buffer, RequestChannel::CLIENT_SIDE);
            }
            else if(ipcMethod == "q"){
                newChan = new MQRequestChannel(buffer, RequestChannel::CLIENT_SIDE);
            }
            else if(ipcMethod == "s"){
                newChan = new SHMRequestChannel(buffer, RequestChannel::CLIENT_SIDE, capacity);   
            }
            char newBuf[MAX_MESSAGE];
            //Creating data for datamsg 
            string newFileName;
            cout << "Enter file to copy: ";
            cin >> newFileName;
            requesting_file(newChan, MAX_MESSAGE, newFileName);
            MESSAGE_TYPE quitmsg = QUIT_MSG;
            newChan->cwrite(&quitmsg, sizeof (MESSAGE_TYPE));
            delete newMsg;
            // .cwrite(&qm, sizeof(MESSAGE_TYPE));
        }
        // closing the channel  
        
        MESSAGE_TYPE m = QUIT_MSG;
        control_channel->cwrite (&m, sizeof (MESSAGE_TYPE));
    }
    wait(0); 
    // cout << result << endl;
}
