#pragma once

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "cache.h"


#define PORT 53
#define BUFFER_SIZE 512

int client_socket;
int server_socket;

// 用于将socket绑定到本地地址
struct sockaddr_in client_address;
struct sockaddr_in server_address;

int address_length;
char *remote_dns; // 远程主机ip地址

WSADATA wsa_data;
SOCKET sock;
// 用于recvform获取服务器和客户端的地址信息
struct sockaddr_in serverAddress;
struct sockaddr_in clientAddress;
int addressLength;

char buffer[BUFFER_SIZE];

void init_socket(int port);
void poll();
void receiveClient();
void receiveServer();

