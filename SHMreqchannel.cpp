#include "common.h"
#include "SHMreqchannel.h"
#include <stdio.h>
#include <stdlib.h>
using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR FOR CLASS   R e q u e s t C h a n n e l  */
/*--------------------------------------------------------------------------*/

SHMRequestChannel::SHMRequestChannel(const string _name, const Side _side, int _length) : RequestChannel (_name, _side){
	s1 = "/SHM_" + my_name + "1";
	s2 = "/SHM_" + my_name + "2";
    length = _length;
    sharedmemqueue1 = new SHMQ(s1, length);
    sharedmemqueue2 = new SHMQ(s2, length);
    if(my_side == CLIENT_SIDE){
        swap(sharedmemqueue1, sharedmemqueue2);
    }
	
}

SHMRequestChannel::~SHMRequestChannel(){ 
    delete sharedmemqueue1;
    delete sharedmemqueue2;
}


int SHMRequestChannel::cread(void* msgbuf, int bufcapacity){
	return sharedmemqueue1->shm_receive(msgbuf, bufcapacity);
}

int SHMRequestChannel::cwrite(void* msgbuf, int len){
	return sharedmemqueue2->shm_send(msgbuf, len);
}