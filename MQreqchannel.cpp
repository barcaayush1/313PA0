#include "common.h"
#include "MQreqchannel.h"
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR FOR CLASS   R e q u e s t C h a n n e l  */
/*--------------------------------------------------------------------------*/

MQRequestChannel::MQRequestChannel(const string _name, const Side _side) : RequestChannel (_name, _side){
	s1 = "/MQ_" + my_name + "1";
	s2 = "/MQ_" + my_name + "2";
	if (_side == SERVER_SIDE){
		wfd = open_ipc(s1, O_RDWR|O_CREAT);
		rfd = open_ipc(s2, O_RDWR|O_CREAT);
	}
	else{
		rfd = open_ipc(s1, O_RDWR|O_CREAT);
		wfd = open_ipc(s2, O_RDWR|O_CREAT);
	}
	
}

MQRequestChannel::~MQRequestChannel(){ 
	mq_close(wfd);
	mq_close(rfd);

	mq_unlink (s1.c_str());
	mq_unlink (s2.c_str());
}

int MQRequestChannel::open_ipc(string _pipe_name, int mode){
	mqd_t mq = mq_open(_pipe_name.c_str(), mode, 0600, NULL);
	if (mq < 0){
		EXITONERROR(_pipe_name);
	}
	return mq;
}

int MQRequestChannel::cread(void* msgbuf, int bufcapacity){
	return mq_receive(rfd, (char *)msgbuf, 8192, NULL); 
}

int MQRequestChannel::cwrite(void* msgbuf, int len){
	return mq_send (wfd, (char *)msgbuf, len, 0);
}
