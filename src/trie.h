#ifndef TRIE_H
#define TRIE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "dnsStruct.h"

// DNS记录结构
typedef struct DNSRecord {
    char domain[DOMAIN_MAX_LEN];
    time_t expire_time;
    uint8_t type;
    union {
        uint32_t ipv4;              // IPv4 地址
        uint8_t ipv6[16];           // IPv6 地址
        char cname[DOMAIN_MAX_LEN]; // CNAME记录
    } value;

    struct DNSRecord* trie_next; // 同域名的下一个记录
    struct DNSRecord* trie_prev; // 同域名的上一个记录
    struct DNSRecord* lru_next; // LRU链表的下一个节点
    struct DNSRecord* lru_prev; // LRU链表的上一个节点
} DNSRecord;

DNSRecord* DNSRecord_create(const char* domain, time_t expire_time, uint8_t type, const void* value);

int DNSRecord_compare(const DNSRecord* a, const DNSRecord* b);

// Trie树的节点结构
typedef struct TrieNode {
    struct TrieNode* children[38];  // 0-9, a-z, -, .
    DNSRecord* head;                // 域名对应的DNS记录链表头指针
    DNSRecord* tail;                // 域名对应的DNS记录链表尾指针
    int isEnd;                      // 是否是域名字符串的末端
    int sum;                        // 包含的有效域名数
} TrieNode;

// 创建一个新的Trie树节点
TrieNode* trie_create();

// 插入域名和IP到Trie树
int trie_insert(TrieNode* root, const char* domain, DNSRecord* record);

// 通过域名查找对应的节点
TrieNode* trie_search(TrieNode* root, const char* domain);

// 在Trie树中删除域名
void trie_delete(TrieNode* root, const char* domain, const DNSRecord* record);

// 清理Trie树释放内存
void trie_free(TrieNode* root);

// 输出路径上的节点信息
void trie_print(TrieNode* root, const char* domain);

#endif // TRIE_H