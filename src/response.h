#pragma once

// 跨平台网络支持
#ifdef _WIN32
    #include <WinSock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "dnsStruct.h"
#include "cache.h"



int build_multi_record_response(unsigned char *buffer,
                                int buf_size,
                                uint16_t transactionID,
                                const char *query_name,
                                uint16_t query_type,
                                CacheQueryResult *first_record);

int build_nxdomain_response(unsigned char *buffer, int buf_size, uint16_t transactionID, const char *query_name, uint16_t query_type);