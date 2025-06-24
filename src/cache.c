#include "cache.h"

DNSCache* cache_create(int capacity) {
    DNSCache* cache = (DNSCache*)malloc(sizeof(DNSCache));
    cache->root = trie_create();
    cache->head = NULL;
    cache->tail = NULL;
    cache->size = 0;
    cache->capacity = capacity;
    return cache;
}

void lru_insert(DNSCache* cache, DNSRecord* record) {
    if (cache->head == NULL) {
        cache->head = record;
        cache->tail = record;
    } else {
        record->lru_prev = cache->tail;
        cache->tail->lru_next = record;
        cache->tail = record;
    }
    cache->size++;
}

void lru_delete(DNSCache* cache, DNSRecord* record) {
    if (record == cache->head && record == cache->tail) {
        cache->head = NULL;
        cache->tail = NULL;
    } else if (record == cache->head) {
        cache->head = record->lru_next;
    } else if (record == cache->tail) {
        cache->tail = record->lru_prev;
    } else {
        record->lru_prev->lru_next = record->lru_next;
        record->lru_next->lru_prev = record->lru_prev;
    }
    cache->size--;
}

void cache_eliminate(DNSCache* cache) {
    DNSRecord* record = cache->head;
    trie_delete(cache->root, record->domain, record);
    lru_delete(cache, record);
    free(record);
}

void cache_update(DNSCache* cache, const char* domain, const uint8_t type, const void* value, time_t ttl) {
    DNSRecord* record = DNSRecord_create(domain, time(NULL) + ttl, type, value);
    if (record == NULL) {
        fprintf(stderr, "Failed to create DNS record\n");
        return;
    }

    DNSRecord* isExist = NULL;
    TrieNode* node = trie_search(cache->root, domain);
    if (node != NULL) {
        DNSRecord* p = node->head;
        while (record != NULL) {
            if (DNSRecord_compare(p, record) == 1) {
                isExist = p;
                break;
            }
            p = p->lru_next;
        }
    }

    if (isExist == NULL) {     // 无相同记录
        if (cache->size == cache->capacity) { // 缓存已满
            cache_eliminate(cache);
        }
        trie_insert(cache->root, domain, record);
        lru_insert(cache, record);
    } else {    // 有相同记录
        lru_delete(cache, isExist);
        trie_delete(cache, domain, isExist);
        free(isExist);
        lru_insert(cache, record);
        trie_insert(cache->root, domain, record);
    }
}

void cache_free(DNSCache* cache) {
    trie_free(cache->root);
    while (cache->head != NULL) {
        DNSCache* node = cache->head;
        cache->head = cache->head->lru_next;
        free(cache->head->lru_prev);
    }
    free(cache);
}