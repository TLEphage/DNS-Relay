#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include <winsock2.h>
// #include <arpa/inet.h> // Not available on Windows
#define MAX_BLACKLIST_SIZE 1000
#define DOMAIN_MAX_LEN 256

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

typedef struct DNS_resource_record{
    char *name;//域名
    uint16_t type;//资源记录类型
    uint16_t class;//资源记录类别
    uint32_t ttl;//生存时间
    uint16_t rdlength;//资源数据长度
    union ResourceData data;//资源数据
}DNS_resource_record;


/*报文结构体*/
typedef struct DNS_message{
    struct DNS_header *header;//报文头部
    struct DNS_question *question;
    struct DNS_resource_record *answer;
    struct DNS_resource_record *authority;
    struct DNS_resource_record *add;
}DNS_message;

union ResourceData{
    struct/*ipv4*/
    {
        uint8_t IP_addr[4];
    }a_record;

    struct/*ipv6*/
    {
        uint8_t IP_addr[16];
    }aaa_record;

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

//Trie树在xn那

/*
typedef struct node{
    uint16_t type;
    char domain[DOMAIN_MAX_LEN];//域名

    union{
        IPAdress ip_addr;
        char cname[DOMAIN_MAX_LEN];
    }data;

    time_t expire_time; // 过期时间
    time_t last_used;//最后使用时间{LRU用}

    struct node*next;
    struct node*lru_prev,*lru_next; // LRU链表指针
}DNSRecord;
*/

/*IP地址结构体*/
/*
typedef struct {
    uint8_t type;
    union{
        uint32_t ipv4;
        uint8_t ipv6[16];
    }addr;
    int isFilledIp4;	// 是否为IPv4地址
	int isFilledIp6;	// 是否为IPv6地址
    struct IPAdress *next; // 下一条ip地址
}IPAdress;
*/

/* ID转换结构体 */
typedef struct {
	uint16_t client_ID;				// 客户端ID
	int expire_time;				// 过期时间
	struct DNS_message* msg;		// DNS报文
	int msg_size;					// 报文大小
	struct sockaddr_in client_addr;	// 客户端地址
} ID_conversion;

typedef struct IPDomainMapping{
    char domain[DOMAIN_MAX_LEN];//域名
    
    time_t expire_time; // 过期时间
    time_t last_used;//最后使用时间{LRU用}
    struct IPDomainMapping *next; // 链表指针

}IPDomainMapping;
//DNS缓存
    typedef struct DNSCache{
        TrieNode *root;
        DNSRecord *lru_head; // LRU链表头
        DNSRecord *lru_tail; // LRU链表尾
        int capacity; // 缓存容量
        int size;   // 当前缓存大小
        IPDomainMapping *ip_domain_head; // IP域名映射表头
        IPDomainMapping *ip_domain_tail; // IP域名映射表尾
    }DNSCache;

    DNSCache *dns_cache;

//IP域名映射

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

/*域名拦截表*/
typedef struct BlackListEntry{
    char domain[DOMAIN_MAX_LEN];//被拦截的域名

    time_t added_time;           //添加到拦截表的时间
}BlackListEntry;

typedef struct DomainBlakcList{
    BlackListEntry entries[MAX_BLACKLIST_SIZE];
    int count;//当前拦截域名数量
}DomainBlackList;

//全局拦截表
extern DomainBlackList *domain_blacklist;

//日志等级
typedef enum{
    LOG_LEVEL_NONE=0,//不打印日志
    LOG_LEVEL_INFO=1,//打印普通信息
    LOG_LEVEL_DEBUG=2,//打印调试信息
}LogLevel;

//IP地址转换
void transferIp(char* originIP, uint8_t* transIP);
void transferIp6(char* originIP, uint16_t* transIP);
int hex_to_int(char c);
