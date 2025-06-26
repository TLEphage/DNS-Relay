#pragma once

#define _WIN32_WINNT 0x0602
#include <WinSock2.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ws2tcpip.h>
#include "cache.h"
#include "response.h"
#include "dnsStruct.h"
#include "log.h"
#include "host.h"

// #pragma comment(lib, "ws2_32.lib")
// #pragma warning(disable : 4996)


#define PORT 53
#define BUFFER_SIZE 512

extern int client_socket;
extern int server_socket;

// 用于将socket绑定到本地地址
extern struct sockaddr_in client_address;
extern struct sockaddr_in server_address;

extern int address_length;
extern char *remote_dns; // 远程主机ip地址

extern WSADATA wsa_data;
extern SOCKET sock;
// 用于recvform获取服务器和客户端的地址信息
extern struct sockaddr_in serverAddress;
extern struct sockaddr_in clientAddress;
extern int addressLength;

extern char buffer[BUFFER_SIZE];

void init();
void poll();
void receiveClient();
void receiveServer();
