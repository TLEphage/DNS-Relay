#pragma once

/*
 * 跨平台兼容性头文件
 * 统一Windows和Linux的网络API、数据类型和常量定义
 */

#ifdef _WIN32
    /* Windows平台 */
    #define _WIN32_WINNT 0x0602
    #include <WinSock2.h>
    #include <ws2tcpip.h>
    #include <io.h>
    #include <direct.h>
    
    /* Windows特定类型定义 */
    typedef SOCKET socket_t;
    typedef int socklen_t;
    typedef WSADATA wsadata_t;
    
    /* Windows特定常量 */
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
    #define SOCKET_ERROR_VALUE SOCKET_ERROR
    #define CLOSE_SOCKET(s) closesocket(s)
    #define GET_SOCKET_ERROR() WSAGetLastError()
    #define WOULD_BLOCK WSAEWOULDBLOCK
    
    /* Windows特定函数宏 */
    #define INIT_NETWORK() init_winsock()
    #define CLEANUP_NETWORK() WSACleanup()
    #define SET_NONBLOCKING(sock) set_nonblocking_win(sock)
    #define POLL_SOCKETS(fds, nfds, timeout) WSAPoll(fds, nfds, timeout)
    
    /* Windows目录操作 */
    #define MKDIR(path) _mkdir(path)
    #define ACCESS(path, mode) _access(path, mode)
    
    /* Windows特定的poll结构 */
    typedef WSAPOLLFD pollfd_t;
    #define POLL_IN POLLIN
    #define POLL_OUT POLLOUT
    #define POLL_ERR POLLERR
    
#else
    /* Linux/Unix平台 */
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <poll.h>
    #include <errno.h>
    #include <sys/stat.h>
    
    /* Linux特定类型定义 */
    typedef int socket_t;
    typedef struct { int dummy; } wsadata_t; // 占位结构
    
    /* Linux特定常量 */
    #define INVALID_SOCKET_VALUE -1
    #define SOCKET_ERROR_VALUE -1
    #define CLOSE_SOCKET(s) close(s)
    #define GET_SOCKET_ERROR() errno
    #define WOULD_BLOCK EWOULDBLOCK
    
    /* Linux特定函数宏 */
    #define INIT_NETWORK() 0  // Linux不需要初始化网络
    #define CLEANUP_NETWORK() // Linux不需要清理网络
    #define SET_NONBLOCKING(sock) set_nonblocking_unix(sock)
    #define POLL_SOCKETS(fds, nfds, timeout) poll(fds, nfds, timeout)
    
    /* Linux目录操作 */
    #define MKDIR(path) mkdir(path, 0755)
    #define ACCESS(path, mode) access(path, mode)
    
    /* Linux的poll结构 */
    typedef struct pollfd pollfd_t;
    #define POLL_IN POLLIN
    #define POLL_OUT POLLOUT
    #define POLL_ERR POLLERR
    
    /* Linux特定的socket选项 */
    #ifndef SO_REUSEADDR
    #define SO_REUSEADDR 2
    #endif
#endif

/* 通用包含文件 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

/* 跨平台函数声明 */
int init_winsock(void);
int set_nonblocking_win(socket_t sock);
int set_nonblocking_unix(socket_t sock);
const char* get_error_string(int error_code);

/* 跨平台网络函数声明 */
socket_t create_udp_socket(void);
int bind_socket(socket_t sock, const struct sockaddr_in* addr);
int set_socket_reuse(socket_t sock);
void close_socket_safe(socket_t sock);
void cleanup_network(void);
int init_network(void);
int set_socket_nonblocking(socket_t sock);
int poll_sockets(pollfd_t* fds, int nfds, int timeout);

/* 跨平台常量定义 */
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

/* 调试宏 */
#ifdef DEBUG
    #define DEBUG_PRINT(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...)
#endif

/* 错误处理宏 */
#define PRINT_SOCKET_ERROR(msg) \
    printf("ERROR: %s: %s\n", msg, get_error_string(GET_SOCKET_ERROR()))
