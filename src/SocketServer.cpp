/*
 * SocketServer.cpp
 *
 *  Created on: Feb 22, 2016
 *      Author: alcht
 */

#include "../head/SocketServer.h"

#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define LogFileName "SocketServer.log"

SocketServer::SocketServer(CallBack * m_callback) :
		pCallback(m_callback) {
	serverPort = 0;
	parallelNumber = 0;
	server_sock_fd = 0;
	client_fds = NULL;
	toDisconnectIndexs = NULL;
	toDiconnectCount = 0;
	messageChain *p;

	logControl.CreateLogFile(LogFileName);

	//recv chain
	p = (messageChain *) malloc(sizeof(messageChain));
	recvMsgInfo.messageHead = p;
	recvMsgInfo.messageTail = p;
	recvMsgInfo.messageCount = 0;
	for (unsigned int i = 1; i < MaxMessNumber; i++) {
		p->next = (messageChain *) malloc(sizeof(messageChain));
		p = p->next;
	}
	p->next = recvMsgInfo.messageHead;

	//recv mutex and sig init
	recvMsgInfo.mutexMessage = PTHREAD_MUTEX_INITIALIZER;
	sem_init(&recvMsgInfo.sigMessage, 0, 0);

	pthread_t threadid;
	pthread_create(&threadid, NULL, ReceiveThread, this);
}

SocketServer::~SocketServer() {
	if (client_fds) {
		free(client_fds);
		client_fds = NULL;
	}
	if (toDisconnectIndexs) {
		free(toDisconnectIndexs);
		toDisconnectIndexs = NULL;
	}
}

int SocketServer::SetServer(unsigned int serverPort, unsigned int parallelNumber) {
	this->serverPort = serverPort;
	this->parallelNumber = parallelNumber;

	client_fds = (int *) malloc(parallelNumber * sizeof(int));
	toDisconnectIndexs = (unsigned int *) malloc(parallelNumber * sizeof(unsigned int));

	if (client_fds && toDisconnectIndexs) {
		memset(client_fds, 0, parallelNumber * sizeof(int));
		memset(toDisconnectIndexs, 0, parallelNumber * sizeof(unsigned int));
		return 0;
	} else {
		if (client_fds) {
			free(client_fds);
			client_fds = NULL;
		}
		if (toDisconnectIndexs) {
			free(toDisconnectIndexs);
			toDisconnectIndexs = NULL;
		}
		return -1;
	}
}

int SocketServer::ForceDisconnectClient(unsigned int index) {
	if (index < parallelNumber) {
		toDisconnectIndexs[toDiconnectCount++] = index;
		return 0;
	}
	return -1;
}
messageNode *SocketServer::aquireNewMessageSpace() {
	messageChain *p;
	if (recvMsgInfo.messageCount == MaxMessNumber) {
		p = NULL;
	} else {
		p = recvMsgInfo.messageTail;
		recvMsgInfo.messageTail = p->next;
		pthread_mutex_lock(&recvMsgInfo.mutexMessage);
		recvMsgInfo.messageCount++;
		pthread_mutex_unlock(&recvMsgInfo.mutexMessage);
	}
	return &p->msgNode;
}

void * SocketServer::ReceiveThread(void * para) {
	SocketServer *socketServer = (SocketServer *) para;
	while (1) {
		sem_wait(&socketServer->recvMsgInfo.sigMessage);
		if (socketServer->recvMsgInfo.messageCount) {
			messageChain *p = socketServer->recvMsgInfo.messageHead;
			socketServer->pCallback->CallbackUpdateMessage(&p->msgNode);
			socketServer->recvMsgInfo.messageHead = p->next;
			pthread_mutex_lock(&socketServer->recvMsgInfo.mutexMessage);
			socketServer->recvMsgInfo.messageCount--;
			pthread_mutex_unlock(&socketServer->recvMsgInfo.mutexMessage);
		} else {
			socketServer->currentTime.getCurrentTime(socketServer->timeBuffer);
			socketServer->logControl.SaveToLog(LogLevelSave, "%s: sem_t has a signal, but message count is 0.", socketServer->timeBuffer);
		}
	}
	return 0;
}

int SocketServer::SendToClient(messageNode *sendMsg) {
	int res = -1;
	if ((sendMsg->index < parallelNumber) && (client_fds[sendMsg->index] > 0)) {
		res = send(client_fds[sendMsg->index], sendMsg->msgData.message, sendMsg->msgData.length, 0);
	}
	return res;
}

int SocketServer::UpdateMessage(char *message, unsigned int messLength, unsigned int index) {
	messageNode *p = aquireNewMessageSpace();
	while (p == NULL) {
		usleep(10000);	//sleep 10ms
		p = aquireNewMessageSpace();
	}
	p->index = index;
	p->msgData.length = messLength;
	memcpy(p->msgData.message, message, messLength);
	sem_post(&recvMsgInfo.sigMessage);
	return 0;
}

int SocketServer::NewClientArrived(unsigned int index, struct sockaddr_in client_address) {
	return pCallback->CallbackNewClientArrived(index, client_address);
}

int SocketServer::ClientLostConnection(unsigned int index) {
	return pCallback->CallbackClientLostConnection(index);
}

int SocketServer::StartServer(void) {
	int reuse = 1;
//������������
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(serverPort);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server_addr.sin_zero), 8);
//������socket
	server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock_fd == -1) {
		perror("socket error");
		return -1;
	}
	if (setsockopt(server_sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		perror("setsockopet error\n");
		return -1;
	}
//������socket
	int bind_result = bind(server_sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
	if (bind_result == -1) {
		perror("bind error");
		return -2;
	}
//listen
	if (listen(server_sock_fd, BACKLOG) == -1) {
		return -3;
	}
	Monitor();
	return 0;
}

void SocketServer::Monitor(void) {
	char input_msg[BUFFER_SIZE];
	char recv_msg[BUFFER_SIZE];

//fd_set
	fd_set server_fd_set;
	int max_fd;
	struct timeval tv; //������������������

	for (unsigned int i = 0; i < parallelNumber; i++) {
		//printf("client_fds[%d]=%d\n", i, client_fds[i]);
		client_fds[i] = 0;
	}

	while (1) {
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		max_fd = -1;

		//check toDiconnectCount
		if (toDiconnectCount) {
			for (unsigned int i = 0; i < toDiconnectCount; i++) {
				close(client_fds[toDisconnectIndexs[i]]);
				client_fds[toDisconnectIndexs[i]] = 0;
			}
			toDiconnectCount = 0;
		}

		FD_ZERO(&server_fd_set);
		//������������socket
		FD_SET(server_sock_fd, &server_fd_set);
		// printf("server_sock_fd=%d\n", server_sock_fd);
		if (max_fd < server_sock_fd) {
			max_fd = server_sock_fd;
		}
		//���������������
		for (unsigned int i = 0; i < parallelNumber; i++) {
			//printf("client_fds[%d]=%d\n", i, client_fds[i]);
			if (client_fds[i] != 0) {
				FD_SET(client_fds[i], &server_fd_set);
				if (max_fd < client_fds[i]) {
					max_fd = client_fds[i];
				}
			}
		}
		int ret = select(max_fd + 1, &server_fd_set, NULL, NULL, &tv);
		if (ret < 0) {
			perror("select ������\n");
			continue;
		} else if (ret == 0) {
			printf("select ������\n");
			continue;
		} else {
			if (FD_ISSET(server_sock_fd, &server_fd_set)) {
				//���������������������
				struct sockaddr_in client_address;
				socklen_t address_len;
				int client_sock_fd = accept(server_sock_fd, (struct sockaddr *) &client_address, &address_len);

				if (client_sock_fd > 0) {
					int index = -1;
					for (unsigned int i = 0; i < parallelNumber; i++) {
						if (client_fds[i] == 0) {
							index = i;
							client_fds[i] = client_sock_fd;
							break;
						}
					}
					if (index >= 0) {
						NewClientArrived(index, client_address);
					} else {
						bzero(input_msg, BUFFER_SIZE);
						strcpy(input_msg, "Server is busy. Please try again\n");
						send(client_sock_fd, input_msg, BUFFER_SIZE, 0);
						currentTime.getCurrentTime(timeBuffer);
						logControl.SaveToLog(LogLevelSave, "%s: Maximum clients count reached, failed to add client: %s:%d.\n", timeBuffer,
								inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
						close(client_sock_fd);
					}
				}
			}
			for (unsigned int i = 0; i < parallelNumber; i++) {
				if (client_fds[i] != 0) {
					if (FD_ISSET(client_fds[i], &server_fd_set)) {
						//������������������������������������
//						bzero(recv_msg, BUFFER_SIZE);
						long byte_num = recv(client_fds[i], recv_msg, BUFFER_SIZE, 0);
						if (byte_num > 0) {
							if (byte_num > BUFFER_SIZE) {
								byte_num = BUFFER_SIZE;
							}
							UpdateMessage(recv_msg, byte_num, i);
						} else if (byte_num < 0) {
							currentTime.getCurrentTime(timeBuffer);
							logControl.SaveToLog(LogLevelSave, "%s: Failed to receive message from client[%d].\n", timeBuffer, i);
						} else {
							FD_CLR(client_fds[i], &server_fd_set);
							ClientLostConnection(i);
							client_fds[i] = 0;
						}
					}
				}
			}
		}
	}
}
