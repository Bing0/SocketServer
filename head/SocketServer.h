/*
 * SocketServer.h
 *
 *  Created on: Feb 22, 2016
 *      Author: alcht
 */

#ifndef SOCKETSERVER_H_
#define SOCKETSERVER_H_

#include <pthread.h>
#include <arpa/inet.h>
#include <semaphore.h>

#include "../head/LogControl.h"
#include "../head/CurrentTime.h"

#define MaxMessNumber	10
#define BACKLOG 5     //完成三次握手但没有accept的队列的长度
#define BUFFER_SIZE 4096

typedef struct {
	unsigned int length;
	char message[BUFFER_SIZE];
}messageData;

typedef struct{
	unsigned int index;
	messageData msgData;
} messageNode;

typedef struct MessageChain {
	struct MessageChain *next;
	messageNode msgNode;
} messageChain;

typedef struct {
	pthread_mutex_t mutexMessage;
	sem_t sigMessage;
	messageChain *messageHead;	//fist message
	messageChain *messageTail;	//latest message
	unsigned int messageCount;
} message_t;

class CallBack {

public:
	virtual ~CallBack() {
	}
	virtual int CallbackUpdateMessage(messageNode *recvMsg) {
		return -1;
	}
	;
	virtual int CallbackNewClientArrived(unsigned int index, struct sockaddr_in client_address) {
		return -1;
	}
	;
	virtual int CallbackClientLostConnection(unsigned int index) {
		return -1;
	}
	;
};

class SocketServer {

public:

	SocketServer(CallBack * m_callback);
	virtual ~SocketServer();
	int SetServer(unsigned int serverPort, unsigned int parallelNumber);
	int StartServer(void);
	int ForceDisconnectClient(unsigned int index);
	int SendToClient(messageNode *sendMsg);

private:
	unsigned int serverPort;
	unsigned int parallelNumber;
	int server_sock_fd;
	int *client_fds;
	unsigned int *toDisconnectIndexs;
	unsigned int toDiconnectCount;
	message_t recvMsgInfo;
	char timeBuffer[20];

	LogControl logControl;
	CurrentTime currentTime;

	messageNode *aquireNewMessageSpace(void);
	int UpdateMessage(char *message, unsigned int messLength, unsigned int index);
	int NewClientArrived(unsigned int index, struct sockaddr_in client_address);
	int ClientLostConnection(unsigned int index);
	void Monitor(void);

private:
	static void * ReceiveThread(void * para);

private:
	CallBack *pCallback;
};

#endif /* SOCKETSERVER_H_ */
