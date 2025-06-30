#include "blacklist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DomainBlacklist* domain_blacklist;

DomainBlacklist* blacklist_create() {
    DomainBlacklist* blacklist = malloc(sizeof(DomainBlacklist));
    if (blacklist == NULL) {
        printf("Failed to allocate memory for blacklist\n");
        return NULL;
    }

    blacklist->count = 0;
    memset(blacklist->entries, 0, sizeof(blacklist->entries));

    return blacklist;
}

void blacklist_update(DomainBlacklist* blacklist, const char* domain) {
    if (blacklist == NULL || domain == NULL) {
        return ;
    }
    if (blacklist->count >= MAX_BLACKLIST_SIZE) {
        printf("Fail to update blacklist: Blacklist is full\n");
        return ;
    }
    // 检查域名是否已存在
    if (blacklist_query(blacklist, domain)) {
        printf("Domain %s is already in blacklist\n", domain);
        return ;  // 已存在，返回成功
    }
    // 添加新域名
    memcpy(blacklist->entries[blacklist->count].domain, domain, strlen(domain));
    blacklist->entries[blacklist->count].domain[strlen(domain)] = '\0';
    blacklist->count++;
    printf("Added domain to blacklist: %s\n", domain);
}

/*
检查域名是否在黑名单中，1表示在
*/
int blacklist_query(DomainBlacklist* blacklist, const char* domain) {
    if (blacklist == NULL || domain == NULL) {
        return 0;
    }
    for (int i = 0; i < blacklist->count; i++) {
        if (strcmp(blacklist->entries[i].domain, domain) == 0) {
            return 1; 
        }
    }
    return 0;
}

void blacklist_destory(DomainBlacklist* blacklist) {
    if (blacklist) {
        free(blacklist);
        printf("Blacklist cleaned up\n");
    }
}