/*
运行：gcc test\test_trie.c src\trie.c -o test\test_trie.exe
*/

#include "trie.h"

// 创建Trie树节点
TrieNode* trie_create() {
    TrieNode* node = (TrieNode*)malloc(sizeof(TrieNode));
    if (node == NULL) {
        fprintf(stderr, "Memory allocation failed for TrieNode\n");
        exit(1);
    }
    for (int i = 0; i < 37; i++) {
        node->children[i] = NULL;
    }
    node->ip = NULL;
    node->isEnd = 0;
    node->sum = 0;
    return node;
}

// 倒序插入域名和IP到Trie树
void trie_insert(TrieNode* root, const char* domain, const char* ip) {
    int len = strlen(domain);
    TrieNode* node = root;
    for (int i = len - 1; i >= 0; i--) {
        int index = -1; 
        if (domain[i]>='0' && domain[i]<='9') index = domain[i] - '0';
        else if(domain[i] >= 'a' && domain[i] <= 'z') {
            index = domain[i] - 'a' + 10;
        } else if(domain[i] >= 'A' && domain[i] <= 'Z') {
            index = domain[i] - 'A' + 10;
        } else if(domain[i] == '.') {
            index = 36;
        } else {
            return;
        }

        if (node->children[index] == NULL) {
            // 如果当前字符不存在，则创建新节点
            node->children[index] = trie_create();
        }
        node = node->children[index];
    }
    // 设置为域名字符串的末端，并存储IP
    node->isEnd = 1;
    node->sum ++;
    if (node->ip != NULL) {
        free(node->ip); // 如果已存在，先释放原有内存
        node->ip = strdup(ip); // 复制IP
    } else {
        node->ip = strdup(ip); // 复制IP
        node = root;
        for (int i = len - 1; i >= 0; i--) {
            node->sum++;

            int index = -1; 
            if (domain[i]>='0' && domain[i]<='9') index = domain[i] - '0';
            else if(domain[i] >= 'a' && domain[i] <= 'z') {
                index = domain[i] - 'a' + 10;
            } else if(domain[i] >= 'A' && domain[i] <= 'Z') {
                index = domain[i] - 'A' + 10;
            } else if(domain[i] == '.') {
                index = 36;
            } else {
                return;
            }

            if (node->children[index] == NULL) {
                // 如果当前字符不存在，则创建新节点
                node->children[index] = trie_create();
            }
            node = node->children[index];
        }
    }
}

void trie_delete(TrieNode* root, const char* domain) {
    int len = strlen(domain);
    TrieNode* node = root;
    for (int i = len - 1; i >= 0; i--) {
        int index = -1; 
        if (domain[i]>='0' && domain[i]<='9') index = domain[i] - '0';
        else if(domain[i] >= 'a' && domain[i] <= 'z') {
            index = domain[i] - 'a' + 10;
        } else if(domain[i] >= 'A' && domain[i] <= 'Z') {
            index = domain[i] - 'A' + 10;
        } else if(domain[i] == '.') {
            index = 36;
        } else {
            return;
        }

        if (node->children[index] == NULL) { // 不存在这个域名
            return ;
        }
        node = node->children[index];
    }
    if (node->isEnd == 0) return ;
    node->isEnd = 0;
    if(node->ip != NULL) {
        free(node->ip);
        node->ip = NULL;
    }
    node = root;
    node->sum--;
    for (int i = len - 1; i >= 0; i--) {
        int index = -1; 
        if (domain[i]>='0' && domain[i]<='9') index = domain[i] - '0';
        else if(domain[i] >= 'a' && domain[i] <= 'z') {
            index = domain[i] - 'a' + 10;
        } else if(domain[i] >= 'A' && domain[i] <= 'Z') {
            index = domain[i] - 'A' + 10;
        } else if(domain[i] == '.') {
            index = 36;
        } else {
            return;
        }

        if (node->children[index]->sum == 1) {
            free(node->children[index]);
            node->children[index] = NULL;
            return ;
        }
        node = node->children[index];
        node->sum--;
    }
}

// 通过域名查找对应的IP
char* trie_search(TrieNode* root, const char* domain) {
    int len = strlen(domain);
    TrieNode* node = root;
    for (int i = len - 1; i >= 0; i--) {
        int index = -1;
        if (domain[i]>='0' && domain[i]<='9') index = domain[i] - '0';
        else if(domain[i] >= 'a' && domain[i] <= 'z') {
            index = domain[i] - 'a' + 10;
        } else if(domain[i] >= 'A' && domain[i] <= 'Z') {
            index = domain[i] - 'A' + 10;
        } else if(domain[i] == '.') {
            index = 36;
        } else {
            return NULL;
        }

        if (node->children[index] == NULL) {
            // 如果找不到匹配的域名，返回NULL
            return NULL;
        }
        node = node->children[index];
    }
    // 如果到达域名字符串末的尾且该节点有效，则返回IP
    return node->isEnd ? node->ip : NULL;
}

// 清理Trie树释放内存
void trie_free(TrieNode* root) {
    if (root == NULL) return;
    
    for (int i = 0; i < 37; i++) {
        if (root->children[i] != NULL) {
            trie_free(root->children[i]); // 递归清理子节点
            root->children[i] = NULL;
        }
    }
    
    // 释放当前节点的IP内存
    if (root->ip != NULL) {
        free(root->ip);
    }
    // 释放当前节点
    free(root);
}

// 输出路径上的信息
void trie_print(TrieNode* root, const char* domain) {
    int len = strlen(domain);
    TrieNode* node = root;
    for (int i = len - 1; i >= 0; i--) {
        int index = -1;
        if (domain[i]>='0' && domain[i]<='9') index = domain[i] - '0';
        else if(domain[i] >= 'a' && domain[i] <= 'z') {
            index = domain[i] - 'a' + 10;
        } else if(domain[i] >= 'A' && domain[i] <= 'Z') {
            index = domain[i] - 'A' + 10;
        } else if(domain[i] == '.') {
            index = 36;
        } else {
            return;
        }

        if (node->children[index] == NULL) {
            return ;
        }
        node = node->children[index];

        printf("domain[i]=%c,sum=%d,isEnd=%d\n", domain[i], node->sum, node->isEnd);
    }
}