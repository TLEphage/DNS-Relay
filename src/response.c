#include "response.h"

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
                                CacheQueryResult *first_record) {
    if (!buffer || !query_name || !first_record) {
        return -1;
    }
    int offset = 0;

    // 计算所有记录的数量（包括CNAME记录）
    int answer_count = 0;
    CacheQueryResult *current = first_record;
    while (current) {
        // 当查询A/AAAA记录时，如果有CNAME链，需要包含CNAME记录和最终的A/AAAA记录
        if (current->record->type == query_type || ((query_type == RR_A || query_type == RR_AAAA) && current->record->type == RR_CNAME)) {
            answer_count++;
        }
        current = current->next;
    }

    if (answer_count == 0) {
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
    while (current) {
        // 当查询A/AAAA记录时，包含CNAME记录和最终的A/AAAA记录
        if (current->record->type == query_type || ((query_type == RR_A || query_type == RR_AAAA) && current->record->type == RR_CNAME)) {
            /*
            对于链表中的第一个记录，它的域名和Question中的域名相同，所以使用指针压缩。写入 0xC00C，0xC0是压缩指针标志，0x0C (十进制12) 指向DNS报文开头偏移12字节的位置，那里正好是Question部分的域名。这能节省空间。
            对于后续的记录（比如CNAME链中的第二个域名），它们的域名与原始查询不同，因此必须完整地编码写入。
            */
            if (current == first_record) {
                // 第一个记录，使用指针压缩
                if (offset + 2 > buf_size) {
                    return -1;
                }
                    
                buffer[offset++] = 0xC0;
                buffer[offset++] = 0x0C; // 指向偏移量12（问题部分的域名）
            } else {
                // 后续记录，写入完整域名
                const char *domain_name = current->record->domain;
                while (*domain_name) {
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
            if (offset + 10 > buf_size) {
                return -1;
            }
            uint16_t type = htons(current->record->type); // 使用当前记录的实际类型
            uint16_t class = htons(1);
            memcpy(buffer + offset, &type, 2);
            offset += 2;
            memcpy(buffer + offset, &class, 2);
            offset += 2;

            time_t current_time = time(NULL);
            uint32_t ttl = (current->record->expire_time > current_time) ? (current->record->expire_time - current_time) : 0;
            uint32_t ttl_net = htonl(ttl);
            memcpy(buffer + offset, &ttl_net, 4);
            offset += 4;

            // 写入RDLENGTH和RDATA
            if (current->record->type == RR_A) {
                if (offset + 6 > buf_size) return -1;
                uint16_t rdlength = htons(4); // A记录长度为4字节
                memcpy(buffer + offset, &rdlength, 2);
                offset += 2;
                memcpy(buffer + offset, &current->record->value.ipv4, 4);
                offset += 4;

                // 打印IP地址调试信息
                uint32_t ip = current->record->value.ipv4;
                printf("  Added a record: %u.%u.%u.%u (TTL: %u)\n", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF, ttl);
            } else if (current->record->type == RR_AAAA) {
                if (offset + 18 > buf_size) return -1;
                uint16_t rdlength = htons(16); // AAAA记录长度为16字节
                memcpy(buffer + offset, &rdlength, 2);
                offset += 2;
                memcpy(buffer + offset, current->record->value.ipv6, 16);
                offset += 16;
                printf("  Added AAAA record (TTL: %u)\n", ttl);
            } else if (current->record->type == RR_CNAME) {
                // CNAME记录处理
                const char *cname = current->record->value.cname;
                int name_wire_len = 0;
                const char *temp_name = cname;

                // 计算编码后的域名长度
                while (*temp_name) {
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

/*
 * 构建DNS NXDOMAIN响应（域名不存在错误）
 * 用于拦截域名时返回给客户端
 */
int build_nxdomain_response(unsigned char *buffer, int buf_size, uint16_t transactionID, const char *query_name, uint16_t query_type)
{
    if (!buffer || !query_name || buf_size < 12) {
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