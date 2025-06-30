#include "host.h"
#include "log.h"
#include <ctype.h>

// 全局统计信息
static HostsStats hosts_stats = {0};

// 验证 IPv4 地址格式
int validate_ipv4(const char* ip_str) {
    if (!ip_str) return 0;
    
    unsigned int b0, b1, b2, b3;
    int result = sscanf(ip_str, "%u.%u.%u.%u", &b0, &b1, &b2, &b3);
    
    if (result != 4) return 0;
    if (b0 > 255 || b1 > 255 || b2 > 255 || b3 > 255) return 0;
    
    // 检查格式是否正确（避免前导零等问题）
    char reconstructed[16];
    snprintf(reconstructed, sizeof(reconstructed), "%u.%u.%u.%u", b0, b1, b2, b3);
    return strcmp(ip_str, reconstructed) == 0;
}

// 验证 IPv6 地址格式
int validate_ipv6(const char* ip_str) {
    if (!ip_str) return 0;
    
    // 简单的验证，检查是否包含合法的 IPv6 字符
    int colon_count = 0;
    int double_colon = 0;
    const char* p = ip_str;
    
    while (*p) {
        if (*p == ':') {
            colon_count++;
            if (*(p+1) == ':') {
                double_colon++;
                if (double_colon > 1) return 0; // 只能有一个 ::
            }
        } else if (!isxdigit(*p)) {
            return 0; // 非法字符
        }
        p++;
    }
    
    // 基本格式检查
    return (colon_count >= 2 && colon_count <= 7);
}


// 解析单行 hosts 文件内容
int parse_hosts_line(char* line, int is_ipv6) {
    if (!line) return -1;
    
    // 移除行末的换行符
    char* newline = strchr(line, '\n');
    if (newline) *newline = '\0';
    
    // 移除行末的回车符
    char* carriage = strchr(line, '\r');
    if (carriage) *carriage = '\0';
    
    // 跳过空行和注释行
    char* trimmed = line;
    while (isspace(*trimmed)) trimmed++;
    if (*trimmed == '\0' || *trimmed == '#') {
        return 0; // 跳过，不算错误
    }
    
    // 解析 IP 地址和域名
    char ip_str[INET6_ADDRSTRLEN];
    char domain[DOMAIN_MAX_LEN];
    char extra_domains[512]; // 存储额外的域名别名
    
    // 初始化缓冲区
    memset(ip_str, 0, sizeof(ip_str));
    memset(domain, 0, sizeof(domain));
    memset(extra_domains, 0, sizeof(extra_domains));
    
    // 使用 sscanf 解析，支持多个域名
    int parsed = sscanf(trimmed, "%s %s %[^\n]", ip_str, domain, extra_domains);
    
    if (parsed < 2) {
        hosts_stats.error_lines++;
        printf("Warning: Invalid hosts line format: %s\n", line);
        return -1;
    }
    
    // 验证 IP 地址格式
    if ((is_ipv6 && !validate_ipv6(ip_str)) || 
        (!is_ipv6 && !validate_ipv4(ip_str))) {
        hosts_stats.error_lines++;
        printf("Warning: Invalid IP address format: %s\n", ip_str);
        return -1;
    }
    
    // 检查是否为拦截地址
    int is_blocked = is_blocked_ip(ip_str, is_ipv6);
    
    // 处理主域名
    if (is_blocked) {
        // 添加到拦截表
        blacklist_update(blacklist, domain);
        printf("Added to blacklist: %s\n", domain);
        hosts_stats.blocked_entries++;
    } else {
        // 添加到 DNS 缓存
        if (is_ipv6) {
            uint8_t ipv6_bytes[16];
            if (inet_pton(AF_INET6, ip_str, ipv6_bytes) == 1) {
                cache_update(dns_cache, domain, RR_AAAA, ipv6_bytes, 86400); // TTL: 24小时
                hosts_stats.ipv6_entries++;
                printf("Added IPv6 cache: %s -> %s\n", domain, ip_str);
            }
        } else {
            uint32_t ipv4_bytes;
            if (inet_pton(AF_INET, ip_str, &ipv4_bytes) == 1) {
                cache_update(dns_cache, domain, RR_A, &ipv4_bytes, 86400); // TTL: 24小时
                hosts_stats.ipv4_entries++;
                printf("Added IPv4 cache: %s -> %s\n", domain, ip_str);
            }
        }
    }
    
    // 处理额外的域名别名
    if (parsed >= 3 && strlen(extra_domains) > 0) {
        char* token = strtok(extra_domains, " \t");
        while (token != NULL) {
            if (strlen(token) > 0) {
                // 添加别名到 DNS 缓存
                if (is_blocked) {
                    blacklist_update(blacklist, token);
                    printf("Added to blacklist: %s\n", token);
                    hosts_stats.blocked_entries++;
                }
                else if (is_ipv6) {
                    uint8_t ipv6_bytes[16];
                    if (inet_pton(AF_INET6, ip_str, ipv6_bytes) == 1) {
                        cache_update(dns_cache, token, RR_AAAA, ipv6_bytes, 86400);
                        hosts_stats.ipv6_entries++;
                        printf("Added IPv6 alias cache: %s -> %s\n", token, ip_str);
                    }
                } else {
                    uint32_t ipv4_bytes;
                    if (inet_pton(AF_INET, ip_str, &ipv4_bytes) == 1) {
                        cache_update(dns_cache, token, RR_A, &ipv4_bytes, 86400);
                        hosts_stats.ipv4_entries++;
                        printf("Added IPv4 alias cache: %s -> %s\n", token, ip_str);
                    }
                }
            }
            token = strtok(NULL, " \t");
        }
    }
    
    hosts_stats.parsed_lines++;
    return 0;
}

// 加载 hosts 文件
int load_hosts_file(const char* filename, int is_ipv6) {
    if (!filename) return -1;
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Warning: Cannot open hosts file: %s\n", filename);
        return -1;
    }
    
    printf("Loading %s file: %s\n", is_ipv6 ? "IPv6" : "IPv4", filename);
    
    char line[1024];
    int line_count = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_count++;
        if (parse_hosts_line(line, is_ipv6) < 0) {
            printf("Error parsing line %d in %s\n", line_count, filename);
        }
    }
    
    fclose(file);
    printf("Finished loading %s, processed %d lines\n", filename, line_count);
    return 0;
}

// 初始化 hosts 文件到 DNS 缓存
void init_hosts_to_dns_cache() {
    printf("Initializing DNS cache from hosts files...\n");
    
    // 重置统计信息
    memset(&hosts_stats, 0, sizeof(hosts_stats));
    
    // 确保 DNS 缓存和拦截表已初始化
    if (!dns_cache) {
        printf("Error: DNS cache not initialized\n");
        return;
    }
    
    // 加载 IPv4 hosts 文件
    if (load_hosts_file(IPV4_FILE_PATH, 0) == 0) {
        printf("Successfully loaded IPv4 hosts file\n");
    }
    
    // 加载 IPv6 hosts 文件
    if (load_hosts_file(IPV6_FILE_PATH, 1) == 0) {
        printf("Successfully loaded IPv6 hosts file\n");
    }
    
    // 打印统计信息
    print_hosts_stats(hosts_stats);
    
}

// 获取统计信息
HostsStats get_hosts_stats() {
    return hosts_stats;
}

// 打印统计信息
void print_hosts_stats(HostsStats stats) {
    printf("\n=== Hosts File Loading Statistics ===\n");
    printf("Parsed lines: %d\n", stats.parsed_lines);
    printf("Error lines: %d\n", stats.error_lines);
    printf("IPv4 entries: %d\n", stats.ipv4_entries);
    printf("IPv6 entries: %d\n", stats.ipv6_entries);
    printf("Total entries: %d\n", stats.ipv4_entries + stats.ipv6_entries + stats.blocked_entries);
    printf("=====================================\n\n");
}

