#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

// 跨平台网络支持
#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif
#define MAX_BLACKLIST_SIZE 1000
#define DOMAIN_MAX_LEN 256

#define RR_A 1
#define RR_AAAA 28
#define RR_CNAME 5

/*报文头部结构体*/
typedef struct DNS_header{
    uint16_t transactionID;//事务ID
    uint16_t flags;//标志位
    uint16_t ques_num;//问题数
    uint16_t ans_num;//回答资源记录数
    uint16_t auth_num;//授权资源记录数
    uint16_t add_num;//附加资源记录数
}DNS_header;


typedef struct DNS_question{
    char *qname;//域名
    uint16_t qtype;//查询类型(如A,AAAA,)
    uint16_t qclass;//查询类别(通常为IN，互联网)
}DNS_question;

union ResourceData{
    struct/*ipv4*/
    {
        uint8_t IP_addr[4];
    }a_record;

    struct/*ipv6*/
    {
        uint8_t IP_addr[16];
    }aaaa_record;

    /* SOA：权威记录的起始 */
    struct
    {
        char *MName;//主服务器域名
        char *RName;//管理员域名
        uint32_t serial;//版本号
        uint32_t refresh;//刷新数据间歇
        uint32_t retry;//重试间隔
        uint32_t expire;//超时重传时间
        uint32_t minimum;//最短有效时间

    }soa_record;

    struct 
    {
        char *name;
    }cname_record;
};

typedef struct DNS_resource_record{
    char *name;//域名
    uint16_t type;//资源记录类型
    uint16_t class;//资源记录类别
    uint32_t ttl;//生存时间
    uint16_t rdlength;//资源数据长度
    union ResourceData data;//资源数据
}DNS_resource_record;


/*
    本头文件专门用于存放DNS报文结构体Message的定义，以及一切有关DNS报文的操作
    DNS 报文格式如下：
    +---------------------+
    |        Header       | 报文头，固定12字节，由结构体DNS_header存储
    +---------------------+
    |       Question      | 向域名服务器的查询请求，由结构体DNS_question存储
    +---------------------+
    |        Answer       | 对于查询问题的回复
    +---------------------+
    |      Authority      | 指向授权域名服务器
    +---------------------+
    |      Additional     | 附加信息
    +---------------------+
    后面三个部分由结构体DNS_resource_record存储
*/
typedef struct DNS_message{
    struct DNS_header *header;//报文头部
    struct DNS_question *question;
    struct DNS_resource_record *answer;
    struct DNS_resource_record *authority;
    struct DNS_resource_record *additional;
}DNS_message;

void parse_dns_packet(DNS_message *msg,const char *buffer,int length);
void parse_resource_record(const char*buffer,int *offset,int max_length,DNS_resource_record *rr);

//转发查询
typedef struct{
    uint16_t orig_id;//客户端原始事务id
    struct sockaddr_in cli;
    time_t timestamp;
}IDEntry;

//转发查询表
typedef struct{
    IDEntry *entries;
    int size;
    int capacity;
}IDTable;


IDTable *id_table;

#define MAX_INFLIGHT 1024   // 最大并发未完成转发请求数
#define QUERY_TIMEOUT_SEC 10 // 超时未得到上游响应

IDEntry ID_list[MAX_INFLIGHT];
bool ID_used[MAX_INFLIGHT];

int find_free_slot(void);

void cleanup_timeouts(void);