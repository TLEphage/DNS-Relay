#include "server.h"
#include "dnsStruct.h"
#include "log.h"

DNSCache *dns_cache;

int main(int argc, char* argv[]) {
    // 解析命令行
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-d")) {
            log_level = LOG_LEVEL_INFO;
        } else if (!strcmp(argv[i], "-dd")) {
            log_level = LOG_LEVEL_DEBUG;
        } else if (!strcmp(argv[i], "-ddd")) {
            log_level = LOG_LEVEL_BYTE;
        }
    }

    if (log_level > LOG_LEVEL_NONE) {  // 日志初始化
        log_init("dnsrelay.log");
        LOG_INFO("Log level = %d\n", log_level);
    }

    print_project_info();

    init();

    dns_poll();

    // 跨平台清理
    CLOSE_SOCKET(sock);
    network_cleanup();
    return 0;
}