/*
用域名建一棵 Trie 树，Trie上每个节点维护一条链表保存资源记录，支持尾部插入和随机删除
使用一条LRU链表将所有资源记录连起来，支持尾部插入和随机删除
头部最老，尾部最新
*/

#ifndef CACHE_H
#define CACHE_H

#include "trie.h"

typedef struct DNSCache {
    TrieNode* root;
    DNSRecord* head; // LRU链表头指针
    DNSRecord* tail; // LRU链表尾指针
    int size;       // 当前大小
    int capacity;   // 最大容量
}DNSCache;

DNSCache* dns_cache;

typedef struct CacheQueryResult {
    DNSRecord* record;
    struct CacheQueryResult* next;
}CacheQueryResult;

DNSCache* cache_create(int capacity);

void lru_insert(DNSCache* cache, DNSRecord* record);

void lru_delete(DNSCache* cache, DNSRecord* record);

void cache_eliminate(DNSCache* cache);

void cache_update(DNSCache* cache, const char* domain, const uint8_t type, const void* value, time_t ttl);

CacheQueryResult* cache_query(DNSCache* cache, const char* domain, const uint8_t type);

void cache_query_free(CacheQueryResult* result);

void cache_destroy(DNSCache* cache);

void cache_print_status(DNSCache* cache);

#endif // CACHE_H