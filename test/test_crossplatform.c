// 简化的跨平台网络测试
#ifdef _WIN32
    #define _WIN32_WINNT 0x0602
    #include <WinSock2.h>
    #include <ws2tcpip.h>
    #include <io.h>

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

    typedef int socket_t;
    #define CLOSE_SOCKET(s) close(s)
    #define GET_SOCKET_ERROR() errno
    #define SOCKET_WOULD_BLOCK EWOULDBLOCK
    #define INVALID_SOCKET_VALUE -1
    #define SOCKET_ERROR_VALUE -1

    #define SOCKET socket_t
    #define WSADATA int
    #define MAKEWORD(a,b) 0
    #define WSAStartup(a,b) 0
    #define WSACleanup() 0
    #define WSAGetLastError() errno
    #define ioctlsocket(s,cmd,arg) fcntl(s, F_SETFL, O_NONBLOCK)
    #define FIONBIO F_SETFL
    #define WSAPoll poll
    #define POLLIN POLLIN
#endif

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

// 跨平台网络初始化
int network_init(void) {
#ifdef _WIN32
    WORD wVersion = MAKEWORD(2, 2);
    WSADATA wsadata;
    return WSAStartup(wVersion, &wsadata);
#else
    return 0;
#endif
}

// 跨平台网络清理
void network_cleanup(void) {
#ifdef _WIN32
    WSACleanup();
#else
    // Linux不需要特殊清理
#endif
}

// 跨平台设置socket为非阻塞模式
int set_socket_nonblocking(socket_t sock) {
#ifdef _WIN32
    unsigned long block_mode = 1;
    return ioctlsocket(sock, FIONBIO, &block_mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif
}

// IP字符串转字节数组
int ip_str_to_bytes_sscanf(const char* ip_str, uint8_t* bytes) {
    if (!ip_str || !bytes)
        return -1;

    unsigned int b0, b1, b2, b3;
    if (sscanf(ip_str, "%u.%u.%u.%u", &b0, &b1, &b2, &b3) != 4) {
        return -1;
    }

    if (b0 > 255 || b1 > 255 || b2 > 255 || b3 > 255) {
        return -1;
    }

    bytes[0] = (uint8_t)b0;
    bytes[1] = (uint8_t)b1;
    bytes[2] = (uint8_t)b2;
    bytes[3] = (uint8_t)b3;

    return 0;
}

// 测试网络初始化和清理
void test_network_init_cleanup() {
    printf("Testing network initialization and cleanup...\n");
    
    int result = network_init();
    assert(result == 0);
    printf("Network initialization successful\n");
    
    network_cleanup();
    printf("Network cleanup successful\n");
}

// 测试socket创建
void test_socket_creation() {
    printf("Testing socket creation...\n");
    
    // 初始化网络
    int result = network_init();
    assert(result == 0);
    
    // 创建socket
    socket_t test_socket = socket(AF_INET, SOCK_DGRAM, 0);
    assert(test_socket != INVALID_SOCKET_VALUE);
    printf("Socket creation successful\n");
    
    // 关闭socket
    CLOSE_SOCKET(test_socket);
    printf("Socket close successful\n");
    
    network_cleanup();
}

// 测试非阻塞设置
void test_nonblocking_socket() {
    printf("Testing non-blocking socket setup...\n");
    
    // 初始化网络
    int result = network_init();
    assert(result == 0);
    
    // 创建socket
    socket_t test_socket = socket(AF_INET, SOCK_DGRAM, 0);
    assert(test_socket != INVALID_SOCKET_VALUE);
    
    // 设置非阻塞
    result = set_socket_nonblocking(test_socket);
    assert(result == 0);
    printf("Non-blocking socket setup successful\n");
    
    // 关闭socket
    CLOSE_SOCKET(test_socket);
    
    network_cleanup();
}

// 测试地址结构
void test_address_structures() {
    printf("Testing address structures...\n");
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(53);
    
    assert(addr.sin_family == AF_INET);
    assert(addr.sin_port == htons(53));
    printf("Address structure setup successful\n");
}

// 测试IP地址转换
void test_ip_conversion() {
    printf("Testing IP address conversion...\n");
    
    uint8_t bytes[4];
    int result = ip_str_to_bytes_sscanf("192.168.1.1", bytes);
    assert(result == 0);
    assert(bytes[0] == 192);
    assert(bytes[1] == 168);
    assert(bytes[2] == 1);
    assert(bytes[3] == 1);
    printf("IP address conversion successful\n");
    
    // 测试无效IP
    result = ip_str_to_bytes_sscanf("256.256.256.256", bytes);
    assert(result == -1);
    printf("Invalid IP address rejection successful\n");
}

// 主测试函数
int main() {
    printf("=== DNS Relay Cross-Platform Test Suite ===\n\n");
    
#ifdef _WIN32
    printf("Running on Windows platform\n");
#else
    printf("Running on Linux/Unix platform\n");
#endif
    
    printf("\n");
    
    test_network_init_cleanup();
    printf("\n");
    
    test_socket_creation();
    printf("\n");
    
    test_nonblocking_socket();
    printf("\n");
    
    test_address_structures();
    printf("\n");
    
    test_ip_conversion();
    printf("\n");
    
    printf("=== All tests passed! ===\n");
    return 0;
}
