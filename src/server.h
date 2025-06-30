#pragma once

// 跨平台网络支持
#ifdef _WIN32
    #define _WIN32_WINNT 0x0602
    #include <WinSock2.h>
    #include <ws2tcpip.h>
    #include <io.h>

    // Windows特定定义
    typedef SOCKET socket_t;
    typedef int socklen_t;
    #define CLOSE_SOCKET(s) closesocket(s)
    #define GET_SOCKET_ERROR() WSAGetLastError()
    #define SOCKET_WOULD_BLOCK WSAEWOULDBLOCK
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
    #define SOCKET_ERROR_VALUE SOCKET_ERROR

#else
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <errno.h>
    #include <fcntl.h>
    #include <poll.h>

    // Linux特定定义
    typedef int socket_t;
    #define CLOSE_SOCKET(s) close(s)
    #define GET_SOCKET_ERROR() errno
    #define SOCKET_WOULD_BLOCK EWOULDBLOCK
    #define INVALID_SOCKET_VALUE -1
    #define SOCKET_ERROR_VALUE -1

    // Windows兼容性定义
    #define SOCKET socket_t
    #define WSADATA int
    #define MAKEWORD(a,b) 0
    #define WSAStartup(a,b) 0
    #define WSACleanup() 0
    #define WSAGetLastError() errno
    #define ioctlsocket(s,cmd,arg) fcntl(s, F_SETFL, O_NONBLOCK)
    #define FIONBIO F_SETFL
    #define WSAPoll poll
    // POLLIN在Linux上已经定义，不需要重新定义

    // pollfd结构在Linux上已经定义
    // #define pollfd pollfd
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cache.h"
#include "blacklist.h"
#include "response.h"
#include "dnsStruct.h"
#include "log.h"
#include "host.h"

// #pragma comment(lib, "ws2_32.lib")
// #pragma warning(disable : 4996)


#define PORT 53
#define BUFFER_SIZE 512

// 跨平台socket变量
socket_t client_socket;
socket_t server_socket;

// 用于将socket绑定到本地地址
struct sockaddr_in client_address;
struct sockaddr_in server_address;

socklen_t address_length;
char *remote_dns; // 远程主机ip地址

// 跨平台兼容变量
WSADATA wsa_data;
socket_t sock;
// 用于recvform获取服务器和客户端的地址信息
struct sockaddr_in serverAddress;
struct sockaddr_in clientAddress;
socklen_t addressLength;

char buffer[BUFFER_SIZE];

// 跨平台网络函数
int network_init(void);
void network_cleanup(void);
int set_socket_nonblocking(socket_t sock);

void init();
void dns_poll();  // 重命名避免与系统poll()函数冲突
void receiveClient();
void receiveServer();
