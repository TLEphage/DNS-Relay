#ifndef TRIE_H
#define TRIE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 定义Trie树的节点结构
typedef struct TrieNode {
    struct TrieNode* children[256]; // 字符集大小(ASCII)
    char* ip;                        // IP字段
    int isEnd;                       // 是否是域名字符串的末端
} TrieNode;

// 创建一个新的Trie树节点
TrieNode* trie_create();

// 插入域名和IP到Trie树
void trie_insert(TrieNode* root, const char* domain, const char* ip);

// 通过域名查找对应的IP
char* trie_search(TrieNode* root, const char* domain);

// 清理Trie树释放内存
void trie_free(TrieNode* root);

#endif // TRIE_H