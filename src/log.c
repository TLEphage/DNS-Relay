#include "log.h"

LogLevel log_level = LOG_LEVEL_NONE;
FILE* log_fp = NULL;

void log_init(const char* path) {
    // 以追加方式打开文件
    log_fp = fopen(path, "a");
    if (!log_fp) {
        perror("Log init error");
    }
}

void log_close(void) {
    if (log_fp) {
        fclose(log_fp);
        log_fp = NULL;
    }
}

void log_write(const char* level_tag, const char* fmt, ...) {
    if (!log_fp) {  // 异常处理
        printf("No log init\n");
        return;
    }

    time_t now = time(NULL);     // 获取当前时间戳
    struct tm tm_now;            // 声明一个结构化时间

    // 跨平台时间转换
#ifdef _WIN32
    localtime_s(&tm_now, &now);  // Windows版本
#else
    localtime_r(&now, &tm_now);  // Linux版本
#endif
    char time_string[20];        // 生成格式化时间字符串
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", &tm_now);

    fprintf(log_fp, "[%s] %s: ", time_string, level_tag);  // 打印日志头部

    va_list ap;                 // 声明一个可变参数读取器
    va_start(ap, fmt);          // 初始化读取器ap，指定从fmt之后的参数开始读
    vfprintf(log_fp, fmt, ap);  // 格式化输出变参内容
    va_end(ap);                 // 结束可变参数处理

    // fprintf(log_fp, "\n");  // 打印换行符
    fflush(log_fp);  // 刷新文件缓冲区，确保数据立即写入
}

void print_project_info(void) {
    printf("==================================================================\n");
    printf("|               Welcome to DNS Server Project                    |\n");
    printf("==================================================================\n");
    printf("| Project: ComputerNetwork-DNS-Server                            |\n");
    printf("| Author:  Group 1                                               |\n");
    printf("|----------------------------------------------------------------|\n");
    printf("| Features:                                                      |\n");
    printf("|    1. Server Function                                          |\n");
    printf("|    2. Relay Function                                           |\n");
    printf("|    3. Bad Website Blocking                                     |\n");
    printf("|    4. Multi-concurrency (ID conversion)                        |\n");
    printf("|    5. Query A/AAAA/CNAME                                       |\n");
    printf("|    6. Non-blocking I/O                                         |\n");
    printf("|    7. Cache Optimization (LRU Algorithm)                       |\n");
    printf("|    8. Query Algorithm Optimization (Trie Tree)                 |\n");
    printf("|----------------------------------------------------------------|\n");
    printf("| Debug Levels:                                                  |\n");
    printf("|    -d   : Level 1 debugging                                    |\n");
    printf("|    -dd  : Level 2 debugging                                    |\n");
    printf("|    -ddd : Level 3 debugging                                    |\n");
    printf("==================================================================\n");
}
