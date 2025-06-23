/*
运行：gcc test\test_trie.c src\trie.c -o test\test_trie.exe
*/

#include "trie.h"

int DNSRecord_compare(const DNSRecord* a, const DNSRecord* b) {
    if(strcmp(a->domain, b->domain)) return 0;
    if(a->type != b->type) return 0;
    if(a->type == RR_A && a->addr.ipv4 != b->addr.ipv4) return 0;
    if(a->type == RR_AAAA && memcmp(a->addr.ipv6, b->addr.ipv6, 16)) return 0;
    if(a->type == RR_CNAME && strcmp(a->addr.cname, b->addr.cname)) return 0;
    return 1;
}

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
    node->head = NULL;
    node->tail = NULL;
    node->isEnd = 0;
    node->sum = 0;
    return node;
}

// 倒序插入域名和IP到Trie树
int trie_insert(TrieNode* root, const char* domain, DNSRecord* record) {
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
            return -1;
        }

        if (node->children[index] == NULL) {
            // 如果当前字符不存在，则创建新节点
            node->children[index] = trie_create();
        }
        node = node->children[index];
    }
    if (node->isEnd == 1) {
        int isExist = 0;
        DNSRecord* p = node->head;
        while(p != NULL) {
            if (DNSRecord_compare(p, record) == 1) {
                isExist = 1;
                break;
            }
            p = p->trie_next;
        }
        if (isExist == 1) return 0; // 已经存在

        node->tail->trie_next = record;
        node->tail->trie_next->trie_next = NULL;
        node->tail->trie_next->trie_prev = node->tail;
        node->tail = node->tail->trie_next;
    } else {
        node->head = record;
        node->tail = record;
        node->head->trie_prev = NULL;
        node->tail->trie_next = NULL;

        node->isEnd = 1;
        node->sum ++;

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
                return -1;
            }

            if (node->children[index] == NULL) {
                // 如果当前字符不存在，则创建新节点
                node->children[index] = trie_create();
            }
            node = node->children[index];
        }
    }
    return 1; // 插入成功
}

// 通过域名查找对应的Trie节点
TrieNode* trie_search(TrieNode* root, const char* domain) {
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
    return node->isEnd ? node : NULL;
}

void trie_delete(TrieNode* root, const char* domain, const DNSRecord* record) {
    int len = strlen(domain);
    TrieNode* node = trie_search(root, domain);
    if (node == NULL) return ;
    
    if (node->isEnd == 0) return ;

    DNSRecord* p = node->head;
    while(p != NULL) {
        if (DNSRecord_compare(p, record) == 1) {
            if (p->trie_prev == NULL) {
                node->head = p->trie_next;
            } else {
                p->trie_prev->trie_next = p->trie_next;
            }
            if (p->trie_next == NULL) {
                node->tail = p->trie_prev;
            } else {
                p->trie_next->trie_prev = p->trie_prev;
            }
            break;
        }
        p = p->trie_next;
    }

    if (node->head != NULL) { // 无需删除这个域名的节点
        return ;
    }

    node->isEnd = 0;
    node->sum--;

    node = root;
    for (int i = len - 1; i >= 0; i--) {
        node->sum--;
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
    }
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
    DNSRecord* p = node->head;
    while(p != NULL) {
        if (p->type == RR_A) {
            printf("A: %u\n", p->addr.ipv4);
        } else if (p->type == RR_AAAA) {
            printf("AAAA: ");
            for (int i = 0; i < 16; i++) 
                printf("%u", p->addr.ipv6[i]);
            putchar('\n');
        } else if (p->type == RR_CNAME) {
            printf("CNAME: %s\n", p->addr.cname);
        }
        p = p->trie_next;
    }
}