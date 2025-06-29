#pragma once

#include "dnsStruct.h"

typedef struct DomainBlacklistEntry {
    char domain[DOMAIN_MAX_LEN];
} DomainBlacklistEntry;

typedef struct DomainBlacklist {
    DomainBlacklistEntry entries[MAX_BLACKLIST_SIZE];
    int count;
} DomainBlacklist;

extern DomainBlacklist* blacklist;

// 创建黑名单
DomainBlacklist* blacklist_create();

// 添加域名
void blacklist_update(DomainBlacklist* blacklist, const char* domain);

// 查询域名是否在黑名单中，1表示在
int blacklist_query(DomainBlacklist* blacklist, const char* domain);

// 清除黑名单
void blacklist_destory(DomainBlacklist* blacklist);