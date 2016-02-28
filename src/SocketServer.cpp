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

SocketServer::SocketServer(CallBack * m_callback) :
		pCallback(m_callback) {
	serverPort = 0;
	parallelNumber = 0;
	server_sock_fd = 0;
	client_fds = NULL;
	toDisconnectIndexs = NULL;
	toDiconnectCount = 0;
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

int SocketServer::SendToClient(char *message, unsigned int messLength, unsigned int index) {
	int res = -1;
	if ((index < parallelNumber) && (client_fds[index] > 0)) {
		res = send(client_fds[index], message, messLength, 0);
	}
	return res;
}
int SocketServer::UpdateMessage(char *message, unsigned int messLength, unsigned int index) {
	return pCallback->CallbackUpdateMessage(message, messLength, index);
}

int SocketServer::NewClientArrived(unsigned int index, struct sockaddr_in client_address) {
	return pCallback->CallbackNewClientArrived(index, client_address);
}

int SocketServer::ClientLostConnection(unsigned int index) {
	return pCallback->CallbackClientLostConnection(index);
}

int SocketServer::StartServer(void) {
	int reuse = 1;
	//本地地址
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(serverPort);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server_addr.sin_zero), 8);
	//创建socket
	server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock_fd == -1) {
		perror("socket error");
		return -1;
	}
	if (setsockopt(server_sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		perror("setsockopet error\n");
		return -1;
	}
	//绑定socket
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
	struct timeval tv; //超时时间设置

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
		//服务器端socket
		FD_SET(server_sock_fd, &server_fd_set);
		// printf("server_sock_fd=%d\n", server_sock_fd);
		if (max_fd < server_sock_fd) {
			max_fd = server_sock_fd;
		}
		//客户端连接
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
			perror("select 出错\n");
			continue;
		} else if (ret == 0) {
			printf("select 超时\n");
			continue;
		} else {
			if (FD_ISSET(server_sock_fd, &server_fd_set)) {
				//有新的连接请求
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
						printf("Maximum clients count reached, failed to add client: %s:%d\n", inet_ntoa(client_address.sin_addr),
								ntohs(client_address.sin_port));
						close(client_sock_fd);
					}
				}
			}
			for (unsigned int i = 0; i < parallelNumber; i++) {
				if (client_fds[i] != 0) {
					if (FD_ISSET(client_fds[i], &server_fd_set)) {
						//处理某个客户端过来的消息
						bzero(recv_msg, BUFFER_SIZE);
						long byte_num = recv(client_fds[i], recv_msg, BUFFER_SIZE, 0);
						if (byte_num > 0) {
							if (byte_num > BUFFER_SIZE) {
								byte_num = BUFFER_SIZE;
							}
							UpdateMessage(recv_msg, byte_num, i);
						} else if (byte_num < 0) {
							printf("Failed to receive message from client[%d].\n", i);
						} else {
							FD_CLR(client_fds[i], &server_fd_set);
							ClientLostConnection(i);
							client_fds[i] = 0;
							printf("Client[%d] exit.\n", i);
						}
					}
				}
			}
		}
	}
}
