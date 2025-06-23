#include "server.h"
#include "dnsStruct.h"


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

/**
 * 构建包含多个相同类型记录的DNS响应（直接生成wire format）
 *
 * 参数：
 *   buffer - 输出缓冲区
 *   buf_size - 缓冲区大小
 *   transactionID - 事务ID
 *   query_name - 查询域名
 *   query_type - 查询类型
 *   first_record - 第一个记录（链表头）
 *
 * 返回值：
 *   成功时返回消息长度，失败返回-1
 */
int build_multi_record_response(unsigned char *buffer,
                                int buf_size,
                                uint16_t transactionID,
                                const char *query_name,
                                uint16_t query_type,
                                DNSRecord *first_record)
{
    if (!buffer || !query_name || !first_record)
    {
        return -1;
    }

    int offset = 0;

    // 计算所有记录的数量（包括CNAME记录）
    int answer_count = 0;
    DNSRecord *current = first_record;
    while (current)
    {
        // 当查询A/AAAA记录时，如果有CNAME链，需要包含CNAME记录和最终的A/AAAA记录
        if (current->type == query_type || ((query_type == RR_A || query_type == RR_AAAA) && current->type == RR_CNAME))
        {
            answer_count++;
        }
        current = current->next;
    }

    if (answer_count == 0)
    {
        return -1;
    }

    printf("Building multi-record response for %s (type=%d), found %d records\n", query_name, query_type, answer_count);

    // 1. 写入DNS头部 (12字节)
    if (offset + 12 > buf_size)
        return -1;

    // 这里通过程序的方式改变字节序(网络字节与CPU相反)
    buffer[offset++] = (transactionID >> 8) & 0xFF;
    buffer[offset++] = transactionID & 0xFF;

    // htons将主机字节序转换为网络字节序
    uint16_t flags = htons(0x8180); // QR=1(响应报), AA=1(权威答案), RD=1(期望递归), RA=1(递归可用), RCODE=0(没有差错)
    memcpy(buffer + offset, &flags, 2);
    offset += 2;

    uint16_t ques_num = htons(1);
    memcpy(buffer + offset, &ques_num, 2);
    offset += 2;

    uint16_t ans_num = htons(answer_count);
    memcpy(buffer + offset, &ans_num, 2);
    offset += 2;

    uint16_t auth_num = 0, add_num = 0;
    memcpy(buffer + offset, &auth_num, 2);
    offset += 2;
    memcpy(buffer + offset, &add_num, 2);
    offset += 2;

    // 2. 写入问题部分
    const char *name = query_name;
    while (*name)
    {
        char *dot = strchr(name, '.'); // 查找当前部分第一个点的位置
        int len = dot ? (dot - name) : strlen(name);

        if (offset + len + 1 > buf_size) {
            return -1;
        }
            
        buffer[offset++] = len;
        memcpy(buffer + offset, name, len);
        offset += len;

        name = dot ? dot + 1 : name + len;
    }
    buffer[offset++] = 0; // 域名结束
    // 存放question当中的域名, www.baidu.com -> 3www5baidu3com0

    if (offset + 4 > buf_size) {
        return -1;
    }
    uint16_t qtype = htons(query_type);
    uint16_t qclass = htons(1); // 1表示IN
    // 存放question当中的type和class
    // 注意：这里的qtype和qclass是网络字节序
    memcpy(buffer + offset, &qtype, 2);
    offset += 2;
    memcpy(buffer + offset, &qclass, 2);
    offset += 2;

    // 3. 写入答案部分（多个记录）
    current = first_record;
    while (current)
    {
        // 当查询A/AAAA记录时，包含CNAME记录和最终的A/AAAA记录
        if (current->type == query_type || ((query_type == RR_A || query_type == RR_AAAA) && current->type == RR_CNAME))
        {
            /*
            对于链表中的第一个记录，它的域名和Question中的域名相同，所以使用指针压缩。写入 0xC00C，0xC0是压缩指针标志，0x0C (十进制12) 指向DNS报文开头偏移12字节的位置，那里正好是Question部分的域名。这能节省空间。
            对于后续的记录（比如CNAME链中的第二个域名），它们的域名与原始查询不同，因此必须完整地编码写入。
            */
            if (current == first_record)
            {
                // 第一个记录，使用指针压缩
                if (offset + 2 > buf_size) {
                    return -1;
                }
                    
                buffer[offset++] = 0xC0;
                buffer[offset++] = 0x0C; // 指向偏移量12（问题部分的域名）
            }
            else
            {
                // 后续记录，写入完整域名
                const char *domain_name = current->domain;
                while (*domain_name)
                {
                    char *dot = strchr(domain_name, '.');
                    int len = dot ? (dot - domain_name) : strlen(domain_name);

                    if (offset + len + 1 > buf_size) {
                        return -1;
                    }
                       
                    buffer[offset++] = len;
                    memcpy(buffer + offset, domain_name, len);
                    offset += len;

                    domain_name = dot ? dot + 1 : domain_name + len;
                }
                buffer[offset++] = 0; // 域名结束
            }

            // 写入TYPE, CLASS, TTL
            if (offset + 10 > buf_size)
                return -1;
            uint16_t type = htons(current->type); // 使用当前记录的实际类型
            uint16_t class = htons(1);
            memcpy(buffer + offset, &type, 2);
            offset += 2;
            memcpy(buffer + offset, &class, 2);
            offset += 2;

            time_t current_time = time(NULL);
            uint32_t ttl = (current->expire_time > current_time) ? (current->expire_time - current_time) : 0;
            uint32_t ttl_net = htonl(ttl);
            memcpy(buffer + offset, &ttl_net, 4);
            offset += 4;

            // 写入RDLENGTH和RDATA
            if (current->type == RR_A)
            {
                if (offset + 6 > buf_size)
                    return -1;
                uint16_t rdlength = htons(4);
                memcpy(buffer + offset, &rdlength, 2);
                offset += 2;
                memcpy(buffer + offset, &current->data.ip_addr.addr.ipv4, 4);
                offset += 4;

                // 打印IP地址调试信息
                uint32_t ip = current->data.ip_addr.addr.ipv4;
                printf("  Added A record: %u.%u.%u.%u (TTL: %u)\n", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF, ttl);
            }
            else if (current->type == RR_AAAA)
            {
                if (offset + 18 > buf_size)
                    return -1;
                uint16_t rdlength = htons(16);
                memcpy(buffer + offset, &rdlength, 2);
                offset += 2;
                memcpy(buffer + offset, current->data.ip_addr.addr.ipv6, 16);
                offset += 16;
                printf("  Added AAAA record (TTL: %u)\n", ttl);
            }
            else if (current->type == RR_CNAME)
            {
                // CNAME记录处理
                const char *cname = current->data.cname;
                int name_wire_len = 0;
                const char *temp_name = cname;

                // 计算编码后的域名长度
                while (*temp_name)
                {
                    char *dot = strchr(temp_name, '.');
                    int label_len = dot ? (dot - temp_name) : strlen(temp_name);
                    name_wire_len += label_len + 1;
                    temp_name = dot ? dot + 1 : temp_name + label_len;
                }
                name_wire_len += 1; // 域名结束符0

                if (offset + 2 + name_wire_len > buf_size)
                    return -1;
                uint16_t rdlength = htons(name_wire_len);
                memcpy(buffer + offset, &rdlength, 2);
                offset += 2;

                // 写入CNAME域名
                const char *name_ptr = cname;
                while (*name_ptr)
                {
                    char *dot = strchr(name_ptr, '.');
                    int len = dot ? (dot - name_ptr) : strlen(name_ptr);

                    buffer[offset++] = len;
                    memcpy(buffer + offset, name_ptr, len);
                    offset += len;

                    name_ptr = dot ? dot + 1 : name_ptr + len;
                }
                buffer[offset++] = 0;

                printf("  Added CNAME record: %s -> %s (TTL: %u)\n", query_name, cname, ttl);
            }
        }
        current = current->next;
    }

    printf("Multi-record response built successfully, total length: %d bytes\n", offset);
    return offset;
}

/**
 * 收集完整的CNAME链，包括CNAME记录和最终的A/AAAA记录
 * 返回一个链接的记录列表，用于构建包含CNAME的完整响应
 * 注意：这个函数创建临时链表，不会修改缓存中的记录
 */
DNSRecord *collect_cname_chain_with_records(DNSCache *cache, const char *domain, uint16_t query_type)
{
    char current_domain[DOMAIN_MAX_LEN];
    strncpy(current_domain, domain, DOMAIN_MAX_LEN - 1);
    current_domain[DOMAIN_MAX_LEN - 1] = '\0';

    int cname_depth = 0;
    const int MAX_CNAME_DEPTH = 10; // 防止CNAME循环

    DNSRecord *first_record = NULL;
    DNSRecord *last_record = NULL;
    bool final_record_found = false; // 新增标志位，追踪是否找到最终记录

    while (cname_depth < MAX_CNAME_DEPTH)
    {
        DNSRecord *record = query_cache(cache, current_domain);
        if (!record)
        {
            break; // 缓存中没有找到，退出循环
        }

        if (record->type == RR_CNAME)
        {
            // 创建CNAME记录的副本并添加到临时链表
            DNSRecord *cname_copy = (DNSRecord *)malloc(sizeof(DNSRecord));
            memcpy(cname_copy, record, sizeof(DNSRecord));
            cname_copy->next = NULL; // 清除原有的next指针

            if (!first_record)
            {
                first_record = cname_copy;
                last_record = cname_copy;
            }
            else
            {
                last_record->next = cname_copy;
                last_record = cname_copy;
            }

            printf("Added CNAME to chain: %s -> %s\n", current_domain, record->data.cname);
            strncpy(current_domain, record->data.cname, DOMAIN_MAX_LEN - 1);
            current_domain[DOMAIN_MAX_LEN - 1] = '\0';
            cname_depth++;
        }
        else if (record->type == query_type)
        {
            // 找到目标类型的记录，收集同一域名的所有该类型记录
            DNSRecord *current_record = record;
            while (current_record)
            {
                if (current_record->type == query_type)
                {
                    // 创建记录的副本并添加到链表末尾
                    DNSRecord *final_copy = (DNSRecord *)malloc(sizeof(DNSRecord));
                    memcpy(final_copy, current_record, sizeof(DNSRecord));
                    final_copy->next = NULL; // 清除原有的next指针

                    if (!first_record)
                    {
                        first_record = final_copy;
                        last_record = final_copy;
                    }
                    else
                    {
                        last_record->next = final_copy;
                        last_record = final_copy;
                    }
                    printf("Added final %s record to chain: %s\n", (query_type == RR_A) ? "A" : "AAAA", current_domain);
                }
                current_record = current_record->next;
            }
            final_record_found = true; // 成功找到最终记录
            break;                     // 找到后退出主循环
        }
        else
        {
            // 找到的记录类型不匹配
            printf("Final domain %s has no %s record in cache\n", current_domain, (query_type == RR_A) ? "A" : "AAAA");
            break;
        }
    }

    if (cname_depth >= MAX_CNAME_DEPTH)
    {
        printf("CNAME chain too deep for %s\n", domain);
    }

    // 只有在完整解析并找到最终记录时才返回结果，否则返回NULL
    if (final_record_found)
    {
        return first_record;
    }
    else
    {
        if (first_record)
        {
            // 如果创建了部分链但未成功，释放内存
            free_temp_record_chain(first_record);
        }
        return NULL;
    }
}

/**
 * 释放由collect_cname_chain_with_records创建的临时链表
 */
void free_temp_record_chain(DNSRecord *first_record)
{
    DNSRecord *current = first_record;
    while (current)
    {
        DNSRecord *next = current->next;
        free(current);
        current = next;
    }
}

/*
 * 构建DNS NXDOMAIN响应（域名不存在错误）
 * 用于拦截域名时返回给客户端
 */
int build_nxdomain_response(unsigned char *buffer, int buf_size, uint16_t transactionID, const char *query_name, uint16_t query_type)
{
    if (!buffer || !query_name || buf_size < 12)
    {
        return -1;
    }

    int offset = 0;

    // DNS Header
    buffer[0] = (transactionID >> 8) & 0xFF; // Transaction ID 高字节
    buffer[1] = transactionID & 0xFF;        // Transaction ID 低字节
    offset += 2;

    // Flags: QR=1(response), OPCODE=0(query), AA=0, TC=0, RD=1, RA=1, Z=000, RCODE=3(NXDOMAIN)
    uint16_t flags = 0x8183; // 1000 0001 1000 0011
    buffer[2] = (flags >> 8) & 0xFF;
    buffer[3] = flags & 0xFF;
    offset += 2;

    // Questions: 1
    buffer[4] = 0x00;
    buffer[5] = 0x01;
    offset += 2;

    // Answer RRs: 0
    buffer[6] = 0x00;
    buffer[7] = 0x00;
    offset += 2;

    // Authority RRs: 0
    buffer[8] = 0x00;
    buffer[9] = 0x00;
    offset += 2;

    // Additional RRs: 0
    buffer[10] = 0x00;
    buffer[11] = 0x00;
    offset += 2;

    // Question section
    // Encode domain name
    const char *p = query_name;
    while (*p)
    {
        size_t len = 0;
        const char *dot = strchr(p, '.');
        if (dot)
        {
            len = dot - p;
        }
        else
        {
            len = strlen(p);
        }

        if (offset + len + 1 > buf_size)
        {
            return -1;
        }

        buffer[offset++] = (char)len;
        memcpy(buffer + offset, p, len);
        offset += len;

        if (dot)
        {
            p = dot + 1;
        }
        else
        {
            p += len;
        }
    }
    buffer[offset++] = '\0'; // Null terminator

    // QTYPE
    if (offset + 2 > buf_size)
    {
        return -1;
    }
    buffer[offset] = (query_type >> 8) & 0xFF;
    buffer[offset + 1] = query_type & 0xFF;
    offset += 2;

    // QCLASS (IN = 1)
    if (offset + 2 > buf_size)
    {
        return -1;
    }
    buffer[offset] = 0x00;
    buffer[offset + 1] = 0x01;
    offset += 2;

    return offset;
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

    // 检查缓存（这里假设只检查第一个问题）
    char *query_name = NULL;
    uint16_t query_type = 0;
    if (msg.header->ques_num > 0 && msg.question != NULL)
    {
        query_name = msg.question[0].qname;
        query_type = msg.question[0].qtype;
    }
    if (query_name != NULL)
    {
        printf("query_name : %s  %d \n", query_name, query_type);
    }

    uint16_t client_txid = msg.header->transactionID;
    // 保存客户端地址以便后续回复
    struct sockaddr_in original_client = clientAddress;

    print_cache_status(dns_cache);

    // 1. 先查询缓存，支持CNAME链解析
    DNSRecord *cached_record = NULL;
    int is_temp_chain = 0; // 标记是否为临时链表

    if (query_name != NULL && (msg.question->qtype == RR_A || msg.question->qtype == RR_AAAA || msg.question->qtype == RR_CNAME))
    {
        if (msg.question->qtype == RR_CNAME)
        {
            // 直接查询CNAME记录
            cached_record = query_cache_all_records(dns_cache, query_name, RR_CNAME);
        }
        else
        {
            // A/AAAA记录使用新的多记录查询
            cached_record = query_cache_all_records(dns_cache, query_name, msg.question->qtype);
            if (!cached_record)
            {
                // 如果没有直接的A/AAAA记录，尝试收集完整的CNAME链
                cached_record = collect_cname_chain_with_records(dns_cache, query_name, msg.question->qtype);
                is_temp_chain = 1; // 标记为临时链表
            }
        }
    }

    // 2. 如果缓存命中
    if (cached_record != NULL)
    {
        printf("Cache hit for: %s\n", query_name);

        // 使用多记录响应构建函数
        int response_len = build_multi_record_response((unsigned char *)buffer, BUF_SIZE, client_txid, query_name, query_type, cached_record);

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
        sendto(client_sock, buffer, response_len, 0, (struct sockaddr *)&original_client, addr_len);

        // 如果使用了临时链表，需要释放
        if (is_temp_chain)
        {
            free_temp_record_chain(cached_record);
        }

        return;
    }
    else
    {
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
        uint16_t new_txid = slot + 1;
        buffer[0] = (new_txid >> 8) & 0xFF;
        buffer[1] = new_txid & 0xFF;

        // 转发请求到远程DNS服务器
        sendto(server_sock, buffer, recvLen, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    }
}

void receiveServer() { 
    int remote_addr_len = sizeof(server_addr);
    int remote_recvLen = recvfrom(server_sock, buffer, BUF_SIZE, 0, (struct sockaddr *)&server_addr, &remote_addr_len);

    if (remote_recvLen > 0)
    {
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
        if (!ID_used[slot])
        {
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
        if (response_msg.header->ques_num > 0 && response_msg.question != NULL)
        {
            query_name = response_msg.question[0].qname;
        }

        // 缓存远程服务器的响应（Answer Section）
        if (query_name != NULL && response_msg.header->ans_num > 0 && response_msg.answer != NULL)
        {
            for (int i = 0; i < response_msg.header->ans_num; i++)
            {
                DNS_resource_record *rr = &response_msg.answer[i];

                if (rr->type == RR_A)
                {
                    uint32_t ipv4_addr;
                    memcpy(&ipv4_addr, rr->data.a_record.IP_addr, 4);
                    // 使用资源记录的实际名称作为缓存键，而不是查询名称
                    add_to_cache(dns_cache, rr->name, RR_A, &ipv4_addr, rr->ttl);
                    printf("Cached A record: %s -> %d.%d.%d.%d, TTL: %u\n", rr->name, rr->data.a_record.IP_addr[0], rr->data.a_record.IP_addr[1],
                           rr->data.a_record.IP_addr[2], rr->data.a_record.IP_addr[3], rr->ttl);
                }
                else if (rr->type == RR_AAAA)
                {
                    uint8_t ipv6_addr[16];
                    memcpy(&ipv6_addr, rr->data.aaaa_record.IP_addr, 16);
                    add_to_cache(dns_cache, rr->name, RR_AAAA, &ipv6_addr, rr->ttl);
                    printf("Cached AAAA record: %s -> [IPv6], TTL: %u\n", rr->name, rr->ttl);
                }
                else if (rr->type == RR_CNAME)
                {
                    // 缓存CNAME记录
                    add_to_cache(dns_cache, rr->name, RR_CNAME, rr->data.cname_record.name, rr->ttl);
                    printf("Cached Additional CNAME record: %s -> %s, TTL: %u\n", rr->name, rr->data.cname_record.name, rr->ttl);
                }
            }
        }

        // 缓存远程服务器的响应（Additional Section）
        if (query_name != NULL && response_msg.header->add_num > 0 && response_msg.additional != NULL)
        {
            printf("Processing %d additional records\n", response_msg.header->add_num);
            for (int i = 0; i < response_msg.header->add_num; i++)
            {
                DNS_resource_record *rr = &response_msg.additional[i];

                if (rr->type == RR_A)
                {
                    uint32_t ipv4_addr;
                    memcpy(&ipv4_addr, rr->data.a_record.IP_addr, 4);
                    add_to_cache(dns_cache, rr->name, RR_A, &ipv4_addr, rr->ttl);
                    printf("Cached Additional A record: %s -> %d.%d.%d.%d, TTL: %u\n", rr->name, rr->data.a_record.IP_addr[0],
                           rr->data.a_record.IP_addr[1], rr->data.a_record.IP_addr[2], rr->data.a_record.IP_addr[3], rr->ttl);
                }
                else if (rr->type == RR_AAAA)
                {
                    uint8_t ipv6_addr[16];
                    memcpy(&ipv6_addr, rr->data.aaaa_record.IP_addr, 16);
                    add_to_cache(dns_cache, rr->name, RR_AAAA, &ipv6_addr, rr->ttl);
                    printf("Cached Additional AAAA record: %s -> [IPv6], TTL: %u\n", rr->name, rr->ttl);
                }
                else if (rr->type == RR_CNAME)
                {
                    add_to_cache(dns_cache, rr->name, RR_CNAME, rr->data.cname_record.name, rr->ttl);
                    printf("Cached Additional CNAME record: %s -> %s, TTL: %u\n", rr->name, rr->data.cname_record.name, rr->ttl);
                }
            }
        }

        // 将响应返回给原始客户端
        sendto(client_sock, buffer, remote_recvLen, 0, (struct sockaddr *)&original_client, addr_len);

        // 释放槽位
        ID_used[slot] = false;
    }
}

/**
 * 释放由collect_cname_chain_with_records创建的临时链表
 */
void free_temp_record_chain(DNSRecord *first_record)
{
    DNSRecord *current = first_record;
    while (current)
    {
        DNSRecord *next = current->next;
        free(current);
        current = next;
    }
}

/**
 * 收集完整的CNAME链，包括CNAME记录和最终的A/AAAA记录
 * 返回一个链接的记录列表，用于构建包含CNAME的完整响应
 * 注意：这个函数创建临时链表，不会修改缓存中的记录
 */
DNSRecord *collect_cname_chain_with_records(DNSCache *cache, const char *domain, uint16_t query_type)
{
    char current_domain[DOMAIN_MAX_LEN];
    strncpy(current_domain, domain, DOMAIN_MAX_LEN - 1);
    current_domain[DOMAIN_MAX_LEN - 1] = '\0';

    int cname_depth = 0;
    const int MAX_CNAME_DEPTH = 10; // 防止CNAME循环

    DNSRecord *first_record = NULL;
    DNSRecord *last_record = NULL;
    bool final_record_found = false; // 新增标志位，追踪是否找到最终记录

    while (cname_depth < MAX_CNAME_DEPTH)
    {
        DNSRecord *record = query_cache(cache, current_domain);
        if (!record)
        {
            break; // 缓存中没有找到，退出循环
        }

        if (record->type == RR_CNAME)
        {
            // 创建CNAME记录的副本并添加到临时链表
            DNSRecord *cname_copy = (DNSRecord *)malloc(sizeof(DNSRecord));
            memcpy(cname_copy, record, sizeof(DNSRecord));
            cname_copy->next = NULL; // 清除原有的next指针

            if (!first_record)
            {
                first_record = cname_copy;
                last_record = cname_copy;
            }
            else
            {
                last_record->next = cname_copy;
                last_record = cname_copy;
            }

            printf("Added CNAME to chain: %s -> %s\n", current_domain, record->data.cname);
            strncpy(current_domain, record->data.cname, DOMAIN_MAX_LEN - 1);
            current_domain[DOMAIN_MAX_LEN - 1] = '\0';
            cname_depth++;
        }
        else if (record->type == query_type)
        {
            // 找到目标类型的记录，收集同一域名的所有该类型记录
            DNSRecord *current_record = record;
            while (current_record)
            {
                if (current_record->type == query_type)
                {
                    // 创建记录的副本并添加到链表末尾
                    DNSRecord *final_copy = (DNSRecord *)malloc(sizeof(DNSRecord));
                    memcpy(final_copy, current_record, sizeof(DNSRecord));
                    final_copy->next = NULL; // 清除原有的next指针

                    if (!first_record)
                    {
                        first_record = final_copy;
                        last_record = final_copy;
                    }
                    else
                    {
                        last_record->next = final_copy;
                        last_record = final_copy;
                    }
                    printf("Added final %s record to chain: %s\n", (query_type == RR_A) ? "A" : "AAAA", current_domain);
                }
                current_record = current_record->next;
            }
            final_record_found = true; // 成功找到最终记录
            break;                     // 找到后退出主循环
        }
        else
        {
            // 找到的记录类型不匹配
            printf("Final domain %s has no %s record in cache\n", current_domain, (query_type == RR_A) ? "A" : "AAAA");
            break;
        }
    }

    if (cname_depth >= MAX_CNAME_DEPTH)
    {
        printf("CNAME chain too deep for %s\n", domain);
    }

    // 只有在完整解析并找到最终记录时才返回结果，否则返回NULL
    if (final_record_found)
    {
        return first_record;
    }
    else
    {
        if (first_record)
        {
            // 如果创建了部分链但未成功，释放内存
            free_temp_record_chain(first_record);
        }
        return NULL;
    }
}