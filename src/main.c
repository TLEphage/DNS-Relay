#include "server.h"
#include "dnsStruct.h"
#include "log.h"


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
        log_init("dnsserver.log");
        LOG_INFO("Log level = %d\n", log_level);
    }

    print_project_info();

    init_socket(PORT);
    init_DNS();

    poll();

    closesocket(sock);
    WSACleanup();
    return 0;
}