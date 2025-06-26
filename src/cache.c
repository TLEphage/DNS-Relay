#include "cache.h"
#include <time.h>

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

    printf("%s\n",domain);
    
    DNSRecord* isExist = NULL;
    TrieNode* node = trie_search(cache->root, domain);
    if (node != NULL) {
        DNSRecord* p = node->head;
        while (p != NULL) {
            if (DNSRecord_compare(p, record) == 1) {
                isExist = p;
                break;
            }
            p = p->trie_next;
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
        trie_delete(cache->root, domain, isExist);
        free(isExist);
        lru_insert(cache, record);
        trie_insert(cache->root, domain, record);
    }
}

CacheQueryResult* cache_query(DNSCache* cache, const char* domain, const uint8_t type) {
    // 构建结果链表
    CacheQueryResult* result = NULL;
    CacheQueryResult* current = NULL;
    
    const int MAX_CNAME_DEPTH = 5;
    int cname_depth = 0;
    
    TrieNode* node = trie_search(cache->root, domain);
    
    // 处理CNAME链，最后得到的node不是CNAME
    while (node != NULL && node->head->type == RR_CNAME) {
        ++cname_depth;
        if(cname_depth > MAX_CNAME_DEPTH) {
            fprintf(stderr, "CNAME loop detected\n");
            cache_query_free(result);
            return NULL;
        }
        if (current == NULL) {
            result = malloc(sizeof(CacheQueryResult));
            current = result;
        } else {
            current->next = malloc(sizeof(CacheQueryResult));
            current = current->next;
        }
        current->record = node->head;
        current->next = NULL;
        node = trie_search(cache->root, node->head->domain);
    }
    
    if (node == NULL) {
        cache_query_free(result);
        return NULL;
    }

    if (type == RR_CNAME) {
        return result;
    }
    
    int isExist = 0;
    DNSRecord* p = node->head;
    
    while (p != NULL) {
        if (p->type == type) {
            if (current == NULL) {
                result = malloc(sizeof(CacheQueryResult));
                current = result;
            } else {
                current->next = malloc(sizeof(CacheQueryResult));
                current = current->next;
            }
            current->record = p;
            current->next = NULL;
            
            isExist = 1;
        }
        p = p->trie_next;
    }
    
    if (!isExist) {
        cache_query_free(result);
        return NULL;
    }
    return result;
}

void cache_query_free(CacheQueryResult* result) {
    CacheQueryResult* p = result;
    while (p != NULL) {
        CacheQueryResult* next = p->next;
        free(p);
        p = next;
    }
}

void cache_destroy(DNSCache* cache) {
    trie_free(cache->root);
    while (cache->head != NULL) {
        DNSRecord* next = cache->head->lru_next;
        free(cache->head);
        cache->head = next;
    }
    free(cache);
}

void cache_print(DNSCache* cache) {
    DNSRecord* p = cache->head;
    int cnt=0;
    while (p != NULL) {
        ++cnt;
        printf("No.%d: %s\n",cnt, p->domain);
        p = p->lru_next;
    }
}