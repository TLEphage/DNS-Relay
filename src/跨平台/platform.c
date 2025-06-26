#include "platform.h"

/*
 * 跨平台网络功能实现
 */

#ifdef _WIN32
/* Windows平台实现 */

int init_winsock(void) {
    WORD wVersion = MAKEWORD(2, 2);
    WSADATA wsadata;
    int result = WSAStartup(wVersion, &wsadata);
    if (result != 0) {
        printf("WSAStartup failed with error: %d\n", result);
        return -1;
    }
    return 0;
}

int set_nonblocking_win(socket_t sock) {
    unsigned long block_mode = 1; // 1 = 非阻塞模式
    int result = ioctlsocket(sock, FIONBIO, &block_mode);
    if (result == SOCKET_ERROR) {
        printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
        return -1;
    }
    return 0;
}

int set_nonblocking_unix(socket_t sock) {
    // Windows下不使用此函数
    (void)sock; // 避免未使用参数警告
    return 0;
}

const char* get_error_string(int error_code) {
    static char error_buffer[256];
    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error_code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        error_buffer,
        sizeof(error_buffer),
        NULL
    );
    return error_buffer;
}

#else
/* Linux/Unix平台实现 */

int init_winsock(void) {
    // Linux不需要初始化网络
    return 0;
}

int set_nonblocking_win(socket_t sock) {
    // Linux下不使用此函数
    (void)sock; // 避免未使用参数警告
    return 0;
}

int set_nonblocking_unix(socket_t sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }
    
    flags |= O_NONBLOCK;
    if (fcntl(sock, F_SETFL, flags) == -1) {
        perror("fcntl F_SETFL O_NONBLOCK");
        return -1;
    }
    return 0;
}

const char* get_error_string(int error_code) {
    return strerror(error_code);
}

#endif

/* 跨平台通用函数 */

socket_t create_udp_socket(void) {
    socket_t sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET_VALUE) {
        PRINT_SOCKET_ERROR("Failed to create socket");
        return INVALID_SOCKET_VALUE;
    }
    return sock;
}

int bind_socket(socket_t sock, const struct sockaddr_in* addr) {
    if (bind(sock, (const struct sockaddr*)addr, sizeof(*addr)) == SOCKET_ERROR_VALUE) {
        PRINT_SOCKET_ERROR("Failed to bind socket");
        return -1;
    }
    return 0;
}

int set_socket_reuse(socket_t sock) {
    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, 
                   (const char*)&reuse, sizeof(reuse)) == SOCKET_ERROR_VALUE) {
        PRINT_SOCKET_ERROR("Failed to set SO_REUSEADDR");
        return -1;
    }
    return 0;
}

void close_socket_safe(socket_t sock) {
    if (sock != INVALID_SOCKET_VALUE) {
        CLOSE_SOCKET(sock);
    }
}

void cleanup_network(void) {
    CLEANUP_NETWORK();
}

int init_network(void) {
    return INIT_NETWORK();
}

int set_socket_nonblocking(socket_t sock) {
#ifdef _WIN32
    return set_nonblocking_win(sock);
#else
    return set_nonblocking_unix(sock);
#endif
}

/* 跨平台的poll包装函数 */
int poll_sockets(pollfd_t* fds, int nfds, int timeout) {
    int result = POLL_SOCKETS(fds, nfds, timeout);
    if (result == SOCKET_ERROR_VALUE) {
        PRINT_SOCKET_ERROR("Poll failed");
        return -1;
    }
    return result;
}
