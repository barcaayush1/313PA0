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
#include <vector>
#include <signal.h>
#include <errno.h>
using namespace std;

class Person{
    public:
    int p;
    double t;
    int e;

    Person(){
        p = 0;
        t = -1.0;
        e = 0;
    }
};

void requesting_file(RequestChannel* channel, int capacity, 
                    string filename, int num_channels, string ipcMethod){   
    //clock_t start, end;               
    //creating a binary file on recieved as the same name that was passed
    string saved_file = "./received/" + filename;
    int std_out = dup(1);
    int file = open((char*)saved_file.c_str(), O_CREAT|O_TRUNC|O_WRONLY, 0777); //creating file for writeonly
    if(!file){ 
        cout << "Not opened" << endl;
    }
    //redirecting stdout to file 
    int windowSize = MAX_MESSAGE;
    if(capacity > 0){
        windowSize = capacity;
    }
    filemsg fm(0, 0);
    char buf[sizeof (filemsg) + filename.size()];
    memcpy (buf, &fm, sizeof (filemsg));
    strcpy (buf + sizeof (filemsg), filename.c_str());
    // memcpy (buf+sizeof(filemsg),fname.c_str(), fname.size()+1);
    channel->cwrite(buf,sizeof (filemsg) + filename.size()+1);
    __int64_t fileLength;
    channel->cread(&fileLength, sizeof(__int64_t));
    //creating new_channel 
    MESSAGE_TYPE qm = QUIT_MSG;
    MESSAGE_TYPE new_chan_msg = NEWCHANNEL_MSG;
    int chunk = fileLength/num_channels;
    pid_t child_pid, wpid;
    int status = 0;
    vector<pid_t> pids;
    for(int i = 0; i < num_channels; i++){ 
        if((child_pid = fork()) == 0)
        {
            channel->cwrite(&new_chan_msg, sizeof(MESSAGE_TYPE));
            char newChan[100];
            channel->cread(&newChan, sizeof(newChan));
            RequestChannel *chan = NULL;
            if(ipcMethod == "f"){
                chan = new FIFORequestChannel(newChan, RequestChannel::CLIENT_SIDE);
            }
            else if(ipcMethod == "q"){
                chan = new MQRequestChannel(newChan, RequestChannel::CLIENT_SIDE);
            }
            else if(ipcMethod == "s"){
                chan = new SHMRequestChannel(newChan, RequestChannel::CLIENT_SIDE, capacity);
            }
            chan->cwrite(&qm, sizeof(MESSAGE_TYPE));
            //exit(0);
        }
    } 
    while(wait(NULL) != -1 || errno != ECHILD){
        cout <<  "waited " << endl;
    };

    channel->cwrite(&qm, sizeof(MESSAGE_TYPE));

}
        
    // }
    // return ; 
    
    // int i = 0;
    // int totalFileRead = 0;
    // while(i < fileLength){
    //     if(totalFileRead+windowSize > fileLength-1){
    //         windowSize = fileLength-totalFileRead;
    //     }
    //     char recvbuf[windowSize];
    //     filemsg fm1(i, windowSize);
    //     memcpy(recvbuf, &fm1, sizeof(filemsg));
    //     memcpy(recvbuf + sizeof(filemsg), filename.c_str(), filename.size() + 1);
    //     channel->cwrite(&recvbuf, sizeof(recvbuf));
    //     channel->cread(&recvbuf, sizeof(recvbuf)+1);
    //     write(file, recvbuf, windowSize);
    //     i = i + windowSize;
    //     totalFileRead += windowSize;
    // }
    // chan->cwrite(&qm, sizeof(MESSAGE_TYPE));

    
// }

int main(int argc, char *argv[]){
    Person person_1;
    int opt;
    int capacity = MAX_MESSAGE;
    string fileName = "";
    bool newChannel = false;
    bool is_file = false;
    string ipcMethod = "f";
    int numChannels = 1;
    char* argument = (char *)to_string(capacity).c_str() ;
    MESSAGE_TYPE qm = QUIT_MSG;
    MESSAGE_TYPE new_chan_msg = NEWCHANNEL_MSG;
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
                is_file = true;
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
        RequestChannel* control_channel = NULL;
        if(ipcMethod == "f"){
            control_channel = new FIFORequestChannel("control", RequestChannel::CLIENT_SIDE);
        }
        else if(ipcMethod == "q"){
            control_channel = new MQRequestChannel("control", RequestChannel::CLIENT_SIDE);
        }
        else if(ipcMethod == "s"){
            control_channel = new SHMRequestChannel("control", RequestChannel::CLIENT_SIDE, capacity);
        }
        if(!is_file){
            if(person_1.t >= 0){
                char buf [MAX_MESSAGE]; //creating a buffer of size MAX_MSG
                datamsg* x = new datamsg (person_1.p, person_1.t, person_1.e); //creating the msg on the heap
                control_channel->cwrite (x, sizeof (datamsg));   //sending the request to the 
                control_channel->cread (&buf, MAX_MESSAGE);
                double reply = *(double *) buf;
                cout << "For person " << person_1.p<< ", at time  " << person_1.t <<", the value of ecg " << person_1.e << " is " << reply << endl;
                delete x;
            } else {
                clock_t start, end;
                //creating and writing using ios::out
                start = clock();
                int i = 0;
                double time = 0;
                double ecgVal;
                const int number_channels = numChannels;
                while(numChannels > 0){
                    RequestChannel * chan = NULL;
                    control_channel->cwrite(&new_chan_msg, sizeof(MESSAGE_TYPE));
                    char new_channel_name[100];
                    control_channel->cread(new_channel_name, sizeof(new_channel_name));
                    if(ipcMethod == "f"){
                        chan = new FIFORequestChannel(new_channel_name, RequestChannel::CLIENT_SIDE);
                    }
                    else if(ipcMethod == "q"){
                        chan = new MQRequestChannel(new_channel_name, RequestChannel::CLIENT_SIDE);
                    }
                    else if(ipcMethod == "s"){
                        chan = new SHMRequestChannel(new_channel_name, RequestChannel::CLIENT_SIDE, capacity);
                    }
                    for(int i = 0; i < 1000/number_channels; i++){
                        datamsg data(person_1.p, time, person_1.e);
                        chan->cwrite(&data, sizeof(data));
                        chan->cread(&ecgVal, sizeof(double));
                        time += 0.004;
                        cout << ecgVal << endl;
                    }
                    //closing the created channel
                    chan->cwrite(&qm, sizeof(MESSAGE_TYPE));
                    numChannels--;
                }
            }
        }
        else{
            requesting_file(control_channel, capacity, fileName, numChannels, ipcMethod);
            return 0;
        }
        control_channel->cwrite (&qm, sizeof (MESSAGE_TYPE));
        wait(0);
    }
   
    return 0;
}