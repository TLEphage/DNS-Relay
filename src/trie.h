#ifndef TRIE_H
#define TRIE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 定义Trie树的节点结构
typedef struct TrieNode {
    struct TrieNode* children[37];  // 0-9, a-z, .
    char* ip;                       // IP字段
    int isEnd;                      // 是否是域名字符串的末端
    int sum;                        // 包含的有效域名数
} TrieNode;

// 创建一个新的Trie树节点
TrieNode* trie_create();

// 插入域名和IP到Trie树
void trie_insert(TrieNode* root, const char* domain, const char* ip);

// 在Trie树中删除域名
void trie_delete(TrieNode* root, const char* domain);

// 通过域名查找对应的IP
char* trie_search(TrieNode* root, const char* domain);

// 清理Trie树释放内存
void trie_free(TrieNode* root);

// 输出路径上的节点信息
void trie_print(TrieNode* root, const char* domain);

#endif // TRIE_H