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

#define BACKLOG 5     //完成三次握手但没有accept的队列的长度
#define BUFFER_SIZE 4096

class CallBack {

public:
	virtual ~CallBack() {
	}
	virtual int CallbackUpdateMessage(char *message, unsigned int messLength, unsigned int index) {
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
	int SendToClient(char *message, unsigned int messLength, unsigned int index);

private:
	unsigned int serverPort;
	unsigned int parallelNumber;
	int server_sock_fd;
	int *client_fds;
	unsigned int *toDisconnectIndexs;
	unsigned int toDiconnectCount;
	int UpdateMessage(char *message, unsigned int messLength, unsigned int index);
	int NewClientArrived(unsigned int index, struct sockaddr_in client_address);
	int ClientLostConnection(unsigned int index);
	void Monitor(void);

private:
	CallBack *pCallback;
};

#endif /* SOCKETSERVER_H_ */
