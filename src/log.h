#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

// 日志等级
typedef enum {
    LOG_LEVEL_NONE = 0,   // 不打印日志P
    LOG_LEVEL_INFO = 1,   // 打印普通信息
    LOG_LEVEL_DEBUG = 2,  // 打印调试信息
    LOG_LEVEL_BYTE = 3    // 打印字节信息
} LogLevel;

extern LogLevel log_level;
extern FILE* log_fp;

// 传入日志文件路径，初始化日志文件指针
void log_init(const char* path);
// 关闭打开的日志文件
void log_close(void);
// 传入日志等级、格式化字符串、所用参数，将信息写入日志文件中
void log_write(const char* level_tag, const char* fmt, ...);
// 打印项目信息
void print_project_info(void);

#define LOG_INFO(fmt, ...)                          \
    do {                                            \
        if (log_level >= LOG_LEVEL_INFO) {          \
            log_write("INFO ", fmt, ##__VA_ARGS__); \
        }                                           \
    } while (0)

#define LOG_DEBUG(fmt, ...)                         \
    do {                                            \
        if (log_level >= LOG_LEVEL_DEBUG) {         \
            log_write("DEBUG", fmt, ##__VA_ARGS__); \
        }                                           \
    } while (0)

#define LOG_BYTE(fmt, ...)                 \
    do {                                   \
        if (log_level >= LOG_LEVEL_BYTE) { \
            printf(fmt, ##__VA_ARGS__);    \
        }                                  \
    } while (0)

#define LOG_ERROR(fmt, ...)                         \
    do {                                            \
        if (log_level >= LOG_LEVEL_INFO) {          \
            log_write("ERROR", fmt, ##__VA_ARGS__); \
        }                                           \
    } while (0)

#define LOG_WARNING(fmt, ...)                       \
    do {                                            \
        if (log_level >= LOG_LEVEL_INFO) {          \
            log_write("WARN ", fmt, ##__VA_ARGS__); \
        }                                           \
    } while (0)
