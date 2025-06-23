#include "server.h"


void init_socket(int port) {
    // 初始化，否则无法运行socket
    WORD wVersion = MAKEWORD(2, 2);
    WSADATA wsadata;
    if (WSAStartup(wVersion, &wsadata) != 0)
    {
        return;
    }

    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&client_address, 0, sizeof(client_address));
    memset(&server_address, 0, sizeof(server_address));

    client_address.sin_family = AF_INET;
    client_address.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY表示本机的任意IP地址
    client_address.sin_port = htons(port);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(remote_dns); // 远程主机地址
    server_address.sin_port = htons(port);

    // 端口复用
    const int REUSE = 1;
    setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&REUSE, sizeof(REUSE));

    if (bind(client_socket, (SOCKADDR *)&client_address, address_length) < 0)
    {
        printf("ERROR: Could not bind: %s\n", strerror(errno));
        exit(-1);
    }

    printf("======================= DNS server running =======================\n");
    printf("| DNS server: %-12s                                       |\n", remote_dns);
    printf("| Listening on port %-12d                                 |\n", port);
    printf("==================================================================\n");
}
void poll() {
    unsigned long block_mode = 1; // 设置为非阻塞模式: I/O操作不会阻塞调用线程
    int server_result = ioctlsocket(server_socket, FIONBIO, &block_mode);
    int client_result = ioctlsocket(client_socket, FIONBIO, &block_mode);

    // 检查是否设置非阻塞失败
    if (server_result == SOCKET_ERROR || client_result == SOCKET_ERROR)
    {
        printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
        closesocket(server_socket);
        closesocket(client_socket);
        WSACleanup();
        return;
    }

    struct pollfd fds[2];

    while (1)
    {
        timeoutHandle();

        fds[0].fd = client_socket;
        fds[0].events = POLLIN;  // POLLIN 表示可读
        fds[1].fd = server_socket;
        fds[1].events = POLLIN;

        // 调用 WSAPoll 进行等待
        int ret = WSAPoll(fds, // fds = 上面填写好的 pollfd 数组
                          2,   // 2 = 数组长度，共两个元素：一个监视客户端，一个监视上游 DNS
                          5    // 5 = 超时时间：5 毫秒。如果 5ms 内没有任何一个 socket 可读，就返回 0
        );

        if (ret == SOCKET_ERROR)
        {
            printf("ERROR WSAPoll: %d.\n", WSAGetLastError());
            LOG_INFO("ERROR WSAPoll: %d.\n", WSAGetLastError());
        }
        else if (ret > 0)
        {
            if (fds[0].revents & POLLIN)
            {
                // client_sock 有数据到达，处理客户端请求
                receiveClient();
            }
            if (fds[1].revents & POLLIN)
            {
                // server_sock 有数据到达，处理服务器响应
                receiveServer();
            }
        }
    }
}

void receiveClient() { 
    DNS_message msg;
    int recv_len = recvfrom(client_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddress, &address_length);
    printf("Received DNS packet from %s:%d, length = %d bytes\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port), recv_len);
    if(recv_len < 0) {
        perror("recvfrom failed");
        return ;
    }

    // 解析 DNS 消息
    parse_dns_packet(&msg, buffer, recv_len);

    
}

void receiveServer() { 
}