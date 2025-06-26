#include "server.h"

int ip_str_to_bytes_sscanf(const char* ip_str, uint8_t* bytes) {
    if (!ip_str || !bytes)
        return -1;

    unsigned int b0, b1, b2, b3;
    if (sscanf(ip_str, "%u.%u.%u.%u", &b0, &b1, &b2, &b3) != 4) {
        return -1;
    }

    // 检查每个字节是否在0-255范围内
    if (b0 > 255 || b1 > 255 || b2 > 255 || b3 > 255) {
        return -1;
    }

    bytes[0] = (uint8_t)b0;
    bytes[1] = (uint8_t)b1;
    bytes[2] = (uint8_t)b2;
    bytes[3] = (uint8_t)b3;

    return 0;
}
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

    address_length = sizeof(client_address);

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

void init_DNS(void) {
    dns_cache = cache_create(1024);
    

    // 从 hosts 文件初始化 DNS 缓存和拦截表
    init_hosts_to_dns_cache();

    // 先手动添加一个默认的DNS记录（作为备用）
    uint32_t ipv4;
    inet_pton(AF_INET, "192.168.1.1", &ipv4);
    ip_str_to_bytes_sscanf("192.168.1.1", (uint8_t*)&ipv4);
    // printf("ipv4 %u\n", ipv4);
    cache_update(dns_cache, "example.com", RR_A, &ipv4, 3600);

    uint8_t ipv6[16];
    // 将文本形式的 IP 地址转换为二进制形式的函数
    inet_pton(AF_INET6, "2001:db8:85a3:0000:0000:8a2e:370:7334", ipv6);
    cache_update(dns_cache, "ipv.example.com", RR_AAAA, ipv6, 7200);
}
void init() {
    remote_dns = "10.3.9.6";
    init_socket(PORT);
    init_DNS();
}

void poll() {
    unsigned long block_mode = 1; // 设置为非阻塞模式: recvform被调用时如果没有数据会立即返回错误，不会阻塞调用线程(主循环)
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
            if (fds[0].revents & POLLIN) //revents是真实发生的事件类型，与POLLIN进行按位与运算可以判断是否有数据可读
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
    // printf("111");
    printf("Received DNS packet from %s:%d, length = %d bytes\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port), recv_len);
    if(recv_len < 0) {
        perror("recvfrom failed");
        return ;
    }
    // printf("222");
    // 解析 DNS 消息
    parse_dns_packet(&msg, buffer, recv_len);
    // printf("333");

    // 检查缓存（这里假设只检查第一个问题）
    char *query_name = NULL;
    uint16_t query_type = 0;
    if (msg.header->ques_num > 0 && msg.question != NULL) {
        query_name = msg.question[0].qname;
        query_type = msg.question[0].qtype;
    }
    if (query_name != NULL) {
        printf("query_name : %s  %d \n", query_name, query_type);
        // printf("444");
    } else {
        return ;
    }

    uint16_t client_txid = msg.header->transactionID;
    // 保存客户端地址以便后续回复
    struct sockaddr_in original_client = clientAddress;

    if(log_level >= LOG_LEVEL_DEBUG) cache_print_status(dns_cache);


    // 1. 先查询缓存，支持CNAME链解析
    CacheQueryResult* query_res;
    query_res = cache_query(dns_cache, query_name, msg.question->qtype);

    // 2. 如果缓存命中
    if (query_res != NULL) {
        printf("Cache hit for: %s\n", query_name);

        // 先判断ip地址中是否有0.0.0.0的不良记录需要拦截
        CacheQueryResult *current = query_res;
        bool is_blocked = false;
        uint8_t zero_ipv6[16] = {0};

        while (current != NULL) {
            if (current->record->type == RR_A && current->record->value.ipv4 == 0) {
                is_blocked = true;
                break;
            } else if(current->record->type == RR_AAAA && memcmp(current->record->value.ipv6, zero_ipv6, 16) == 0) {
                is_blocked = true;
                break;
            }
            current = current->next;
        }

        if (is_blocked) {
            printf("Domain %s is BLOCKED, returning NXDOMAIN response\n", query_name);

            // 构建NXDOMAIN响应
            int response_len = build_nxdomain_response((unsigned char *)buffer, BUFFER_SIZE, client_txid, query_name, query_type);
            if (response_len > 0) {
                // 发送NXDOMAIN响应给客户端
                sendto(client_socket, buffer, response_len, 0, (struct sockaddr *)&original_client, address_length);
                printf("Sent NXDOMAIN response for blocked domain: %s\n", query_name);
            } else {
                printf("Failed to build NXDOMAIN response for: %s\n", query_name);
            }
            cache_query_free(query_res);
            return ;
        } else {
            // 使用多记录响应构建函数
            int response_len = build_multi_record_response((unsigned char *)buffer, BUFFER_SIZE, client_txid, query_name, query_type, query_res);

            // 如果是CNAME或者RR_A查询，打印要发送的字节数据
            if ((query_type == RR_CNAME || query_type == RR_A) && response_len > 0)
            {
                LOG_BYTE("=== CNAME Response Bytes Debug (Length: %d) ===\n", response_len);
                for (int i = 0; i < response_len; i++)
                {
                    LOG_BYTE("%02X ", (unsigned char)buffer[i]);
                    if ((i + 1) % 16 == 0)
                        LOG_BYTE("\n"); // 每16字节换行
                }
                if (response_len % 16 != 0)
                    LOG_BYTE("\n"); // 最后一行换行

                // 也打印ASCII可读部分
                LOG_BYTE("ASCII representation:\n");
                for (int i = 0; i < response_len; i++)
                {
                    char c = buffer[i];
                    if (c >= 32 && c <= 126)
                    {
                        LOG_BYTE("%c", c);
                    }
                    else
                    {
                        LOG_BYTE(".");
                    }
                    if ((i + 1) % 64 == 0)
                        LOG_BYTE("\n"); // 每64字符换行
                }
                if (response_len % 64 != 0)
                    LOG_BYTE("\n");
                LOG_BYTE("===============================================\n");
            }

            // 发送缓存响应给客户端
            sendto(client_socket, buffer, response_len, 0, (struct sockaddr *)&original_client, address_length);
        }
        cache_query_free(query_res);
    } else {
        printf("Cache miss for: %s, forwarding to remote DNS\n", query_name);

        // 查找空闲槽位
        cleanup_timeouts();
        int slot = find_free_slot();
        if (slot == -1)
        {
            printf("No free slots available, dropping request\n");
            return;
        }

        // 保存事务ID和客户端信息
        ID_list[slot].orig_id = client_txid;
        ID_list[slot].cli = original_client;
        ID_list[slot].timestamp = time(NULL);
        ID_used[slot] = true;

        // 修改事务ID为槽位索引+1，避免冲突
        uint16_t new_txid = (slot % 0xFFFF) + 1;
        buffer[0] = (new_txid >> 8) & 0xFF;
        buffer[1] = new_txid & 0xFF;

        printf("DEBUG: Before sendto: server_address.sin_family = %d (should be 2), IP = %s\n",
           server_address.sin_family, inet_ntoa(server_address.sin_addr));

        // 转发请求到远程DNS服务器
        if(sendto(server_socket, buffer, recv_len, 0, (struct sockaddr *)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
            printf("Error sending request to remote DNS server\n");
            LOG_ERROR("Sendto failed: %d", WSAGetLastError());
            ID_used[slot] = false; // 立即释放槽位
        }
    }
}

void receiveServer() { 
    int remote_addr_len = sizeof(server_address);
    int remote_recvLen = recvfrom(server_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_address, &remote_addr_len);

    if (remote_recvLen > 0) {
        printf("Received response from remote DNS, length = %d bytes\n", remote_recvLen);

        // 获取服务器响应中的事务ID
        uint16_t server_txid = (buffer[0] << 8) | buffer[1];

        // 检查事务ID是否有效
        if (server_txid == 0 || server_txid > MAX_INFLIGHT)
        {
            printf("Invalid transaction ID in response: %d\n", server_txid);
            return;
        }

        int slot = server_txid - 1; // 转换为槽位索引，由于事务ID是通过数组的槽位加一产生，所以这里需要减一

        // 检查事务ID是否在使用中
        if (!ID_used[slot]) {
            printf("No matching request found for transaction ID %d\n", server_txid);
            return;
        }

        // 恢复原始事务ID，将从服务器返回的ID映射到原始客户端的事务ID，并发送到服务器
        uint16_t orig_txid = ID_list[slot].orig_id;
        buffer[0] = (orig_txid >> 8) & 0xFF;
        buffer[1] = orig_txid & 0xFF;

        // 获取原始客户端地址
        struct sockaddr_in original_client = ID_list[slot].cli;

        // 解析DNS报文以获取查询名和响应记录
        DNS_message response_msg;
        parse_dns_packet(&response_msg, buffer, remote_recvLen);

        char *query_name = NULL;
        if (response_msg.header->ques_num > 0 && response_msg.question != NULL) {
            query_name = response_msg.question[0].qname;
        }

        // 缓存远程服务器的响应（Answer Section）
        if (query_name != NULL && response_msg.header->ans_num > 0 && response_msg.answer != NULL) {
            for (int i = 0; i < response_msg.header->ans_num; i++) {
                DNS_resource_record *rr = &response_msg.answer[i];

                if (rr->type == RR_A) {
                    uint32_t ipv4_addr;
                    memcpy(&ipv4_addr, rr->data.a_record.IP_addr, 4);
                    // 使用资源记录的实际名称作为缓存键，而不是查询名称
                    cache_update(dns_cache, rr->name, RR_A, &ipv4_addr, rr->ttl);
                    printf("Cached A record: %s -> %d.%d.%d.%d, TTL: %u\n", rr->name, rr->data.a_record.IP_addr[0], rr->data.a_record.IP_addr[1],
                           rr->data.a_record.IP_addr[2], rr->data.a_record.IP_addr[3], rr->ttl);
                }
                else if (rr->type == RR_AAAA) {
                    uint8_t ipv6_addr[16];
                    memcpy(&ipv6_addr, rr->data.aaaa_record.IP_addr, 16);
                    cache_update(dns_cache, rr->name, RR_AAAA, &ipv6_addr, rr->ttl);
                    printf("Cached AAAA record: %s -> [IPv6], TTL: %u\n", rr->name, rr->ttl);
                }
                else if (rr->type == RR_CNAME) {
                    // 缓存CNAME记录
                    cache_update(dns_cache, rr->name, RR_CNAME, rr->data.cname_record.name, rr->ttl);
                    printf("Cached Additional CNAME record: %s -> %s, TTL: %u\n", rr->name, rr->data.cname_record.name, rr->ttl);
                }
            }
        }

        // 缓存远程服务器的响应（Additional Section）
        if (query_name != NULL && response_msg.header->add_num > 0 && response_msg.additional != NULL) {
            printf("Processing %d additional records\n", response_msg.header->add_num);
            for (int i = 0; i < response_msg.header->add_num; i++) {
                DNS_resource_record *rr = &response_msg.additional[i];

                if (rr->type == RR_A) {
                    uint32_t ipv4_addr;
                    memcpy(&ipv4_addr, rr->data.a_record.IP_addr, 4);
                    cache_update(dns_cache, rr->name, RR_A, &ipv4_addr, rr->ttl);
                    printf("Cached Additional A record: %s -> %d.%d.%d.%d, TTL: %u\n", rr->name, rr->data.a_record.IP_addr[0],
                           rr->data.a_record.IP_addr[1], rr->data.a_record.IP_addr[2], rr->data.a_record.IP_addr[3], rr->ttl);
                } else if (rr->type == RR_AAAA) {
                    uint8_t ipv6_addr[16];
                    memcpy(&ipv6_addr, rr->data.aaaa_record.IP_addr, 16);
                    cache_update(dns_cache, rr->name, RR_AAAA, &ipv6_addr, rr->ttl);
                    printf("Cached Additional AAAA record: %s -> [IPv6], TTL: %u\n", rr->name, rr->ttl);
                } else if (rr->type == RR_CNAME) {
                    cache_update(dns_cache, rr->name, RR_CNAME, rr->data.cname_record.name, rr->ttl);
                    printf("Cached Additional CNAME record: %s -> %s, TTL: %u\n", rr->name, rr->data.cname_record.name, rr->ttl);
                }
            }
        }

        // 将响应返回给原始客户端
        sendto(client_socket, buffer, remote_recvLen, 0, (struct sockaddr *)&original_client, address_length);

        // 释放槽位
        ID_used[slot] = false;
    }
}
