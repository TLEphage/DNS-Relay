/*
 * DNS中继器性能基准测试
 * 用于验证Windows和Linux版本的性能一致性
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #define _WIN32_WINNT 0x0602
    #include <WinSock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    
    typedef SOCKET socket_t;
    #define CLOSE_SOCKET(s) closesocket(s)
    #define GET_SOCKET_ERROR() WSAGetLastError()
    
    // Windows高精度计时
    static LARGE_INTEGER freq;
    static int timer_init = 0;
    
    double get_time() {
        if (!timer_init) {
            QueryPerformanceFrequency(&freq);
            timer_init = 1;
        }
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        return (double)counter.QuadPart / freq.QuadPart;
    }
    
    void sleep_ms(int ms) {
        Sleep(ms);
    }
    
#else
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <errno.h>
    #include <sys/time.h>
    
    typedef int socket_t;
    #define CLOSE_SOCKET(s) close(s)
    #define GET_SOCKET_ERROR() errno
    #define INVALID_SOCKET -1
    
    // Linux高精度计时
    double get_time() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec + tv.tv_usec / 1000000.0;
    }
    
    void sleep_ms(int ms) {
        usleep(ms * 1000);
    }
    
#endif

// 网络初始化
int network_init() {
#ifdef _WIN32
    WSADATA wsadata;
    return WSAStartup(MAKEWORD(2, 2), &wsadata);
#else
    return 0;
#endif
}

// 网络清理
void network_cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// 创建DNS查询包
int create_dns_query(unsigned char* buffer, const char* domain, unsigned short id) {
    int pos = 0;
    
    // DNS头部
    buffer[pos++] = (id >> 8) & 0xFF;  // ID高字节
    buffer[pos++] = id & 0xFF;         // ID低字节
    buffer[pos++] = 0x01;              // 标志：标准查询
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;              // 问题数
    buffer[pos++] = 0x01;
    buffer[pos++] = 0x00;              // 回答数
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;              // 权威数
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;              // 附加数
    buffer[pos++] = 0x00;
    
    // 域名编码
    const char* start = domain;
    const char* end;
    while (*start) {
        end = strchr(start, '.');
        if (!end) end = start + strlen(start);
        
        int len = end - start;
        buffer[pos++] = len;
        memcpy(buffer + pos, start, len);
        pos += len;
        
        if (*end == '.') start = end + 1;
        else break;
    }
    buffer[pos++] = 0x00;  // 域名结束
    
    // 查询类型和类
    buffer[pos++] = 0x00;  // 类型：A记录
    buffer[pos++] = 0x01;
    buffer[pos++] = 0x00;  // 类：IN
    buffer[pos++] = 0x01;
    
    return pos;
}

// 性能测试结构
typedef struct {
    int total_queries;
    int successful_queries;
    int failed_queries;
    double total_time;
    double min_time;
    double max_time;
    double avg_time;
} benchmark_result_t;

// 单次DNS查询测试
int test_single_query(const char* server_ip, int server_port, const char* domain, double* response_time) {
    socket_t sock;
    struct sockaddr_in server_addr;
    unsigned char query_buffer[512];
    unsigned char response_buffer[512];
    int query_len;
    double start_time, end_time;
    
    // 创建socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        return -1;
    }
    
    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    
    // 创建DNS查询
    query_len = create_dns_query(query_buffer, domain, rand() % 65536);
    
    // 发送查询并计时
    start_time = get_time();
    
    if (sendto(sock, (char*)query_buffer, query_len, 0, 
               (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        CLOSE_SOCKET(sock);
        return -1;
    }
    
    // 接收响应
    int recv_len = recvfrom(sock, (char*)response_buffer, sizeof(response_buffer), 0, NULL, NULL);
    
    end_time = get_time();
    
    CLOSE_SOCKET(sock);
    
    if (recv_len > 0) {
        *response_time = (end_time - start_time) * 1000.0;  // 转换为毫秒
        return 0;
    }
    
    return -1;
}

// 运行基准测试
void run_benchmark(const char* server_ip, int server_port, const char* test_name, 
                   const char** domains, int domain_count, int iterations, 
                   benchmark_result_t* result) {
    
    printf("Running %s benchmark...\n", test_name);
    printf("Target: %s:%d\n", server_ip, server_port);
    printf("Domains: %d, Iterations per domain: %d\n", domain_count, iterations);
    printf("Total queries: %d\n\n", domain_count * iterations);
    
    memset(result, 0, sizeof(benchmark_result_t));
    result->min_time = 999999.0;
    result->max_time = 0.0;
    
    double total_start_time = get_time();
    
    for (int d = 0; d < domain_count; d++) {
        printf("Testing domain: %s\n", domains[d]);
        
        for (int i = 0; i < iterations; i++) {
            double response_time;
            
            if (test_single_query(server_ip, server_port, domains[d], &response_time) == 0) {
                result->successful_queries++;
                result->total_time += response_time;
                
                if (response_time < result->min_time) result->min_time = response_time;
                if (response_time > result->max_time) result->max_time = response_time;
                
                printf(".");
                fflush(stdout);
            } else {
                result->failed_queries++;
                printf("x");
                fflush(stdout);
            }
            
            // 小延迟避免过载
            sleep_ms(10);
        }
        printf(" (%d/%d successful)\n", 
               result->successful_queries - (d * iterations), iterations);
    }
    
    double total_end_time = get_time();
    
    result->total_queries = domain_count * iterations;
    if (result->successful_queries > 0) {
        result->avg_time = result->total_time / result->successful_queries;
    }
    
    printf("\n=== %s Benchmark Results ===\n", test_name);
    printf("Total queries:     %d\n", result->total_queries);
    printf("Successful:        %d (%.1f%%)\n", 
           result->successful_queries, 
           (double)result->successful_queries / result->total_queries * 100.0);
    printf("Failed:            %d (%.1f%%)\n", 
           result->failed_queries,
           (double)result->failed_queries / result->total_queries * 100.0);
    printf("Total time:        %.2f seconds\n", total_end_time - total_start_time);
    printf("Average response:  %.2f ms\n", result->avg_time);
    printf("Min response:      %.2f ms\n", result->min_time);
    printf("Max response:      %.2f ms\n", result->max_time);
    printf("Queries per sec:   %.1f\n", 
           result->successful_queries / (total_end_time - total_start_time));
    printf("\n");
}

int main() {
    printf("=== DNS Relay Performance Benchmark ===\n\n");
    
#ifdef _WIN32
    printf("Platform: Windows\n");
#else
    printf("Platform: Linux\n");
#endif
    
    // 初始化网络
    if (network_init() != 0) {
        printf("Failed to initialize network\n");
        return 1;
    }
    
    // 测试域名列表
    const char* test_domains[] = {
        "google.com",
        "baidu.com", 
        "github.com",
        "stackoverflow.com",
        "wikipedia.org"
    };
    int domain_count = sizeof(test_domains) / sizeof(test_domains[0]);
    
    benchmark_result_t local_result, external_result;
    
    // 测试本地DNS中继器
    printf("Testing local DNS relay (127.0.0.1:53)...\n");
    run_benchmark("127.0.0.1", 53, "Local DNS Relay", 
                  test_domains, domain_count, 10, &local_result);
    
    // 测试外部DNS服务器作为对比
    printf("Testing external DNS server (8.8.8.8:53)...\n");
    run_benchmark("8.8.8.8", 53, "External DNS Server", 
                  test_domains, domain_count, 10, &external_result);
    
    // 性能对比
    printf("=== Performance Comparison ===\n");
    if (local_result.successful_queries > 0 && external_result.successful_queries > 0) {
        double speedup = external_result.avg_time / local_result.avg_time;
        printf("Local relay avg response: %.2f ms\n", local_result.avg_time);
        printf("External DNS avg response: %.2f ms\n", external_result.avg_time);
        printf("Performance ratio: %.2fx %s\n", 
               speedup > 1.0 ? speedup : 1.0/speedup,
               speedup > 1.0 ? "faster" : "slower");
    }
    
    network_cleanup();
    
    printf("\nBenchmark completed.\n");
    return 0;
}
