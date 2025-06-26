#pragma once

#include "platform.h"
#include "cache.h"
#include "response.h"
#include "dnsStruct.h"
#include "log.h"
#include "host.h"

// #pragma comment(lib, "ws2_32.lib")
// #pragma warning(disable : 4996)


#define PORT 53
#define BUFFER_SIZE 512

extern socket_t client_socket;
extern socket_t server_socket;

// 用于将socket绑定到本地地址
extern struct sockaddr_in client_address;
extern struct sockaddr_in server_address;

extern socklen_t address_length;
extern char *remote_dns; // 远程主机ip地址

extern wsadata_t wsa_data;
extern socket_t sock;
// 用于recvform获取服务器和客户端的地址信息
extern struct sockaddr_in serverAddress;
extern struct sockaddr_in clientAddress;
extern socklen_t addressLength;

extern char buffer[BUFFER_SIZE];

void init();
void poll();
void receiveClient();
void receiveServer();
