#pragma once

// 跨平台网络头文件
#ifdef _WIN32
    #include <WinSock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

#include "cache.h"
#include "blacklist.h"
// 默认的 hosts 文件路径
#define IPV4_FILE_PATH "dnsrelay.txt"
#define IPV6_FILE_PATH "dnsrelay_ipv6.txt"

// hosts 文件解析结果统计
typedef struct HostsStats {
    int ipv4_entries;      // IPv4 条目数量
    int ipv6_entries;      // IPv6 条目数量  
    int blocked_entries;   // 被拦截的条目数量
    int parsed_lines;      // 解析的行数
    int error_lines;       // 错误行数
} HostsStats;

// 函数声明
int load_hosts_file(const char* filename, int is_ipv6);
int parse_hosts_line(char* line, int is_ipv6);
int is_blocked_ip(const char* ip_str, int is_ipv6);
void init_hosts_to_dns_cache();
HostsStats get_hosts_stats();
void print_hosts_stats(HostsStats stats);
int validate_ipv4(const char* ip_str);
int validate_ipv6(const char* ip_str);











