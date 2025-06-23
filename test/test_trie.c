#include "../src/trie.h"

int main() {
    TrieNode* root = trie_create();
    trie_insert(root, "www.google.com", "1.1.1.1");
    trie_insert(root, "www.google.com", "2.2.2.2");
    trie_insert(root, "www.baidu.com", "3.3.3.3");
    trie_insert(root, "ww.baidu.com", "4.4.4.4");
    trie_insert(root, "123.123", "4.4.4.5");

    char* response = trie_search(root, "www.google.com");
    if (response!=NULL) printf("%s\n", response);
    else printf("none\n");

    response = trie_search(root, "www.baidu.com");
    if (response!=NULL) printf("%s\n", response);
    else printf("none\n");

    response = trie_search(root, "ww.Baidu.com");
    if (response!=NULL) printf("%s\n", response);
    else printf("none\n");

    response = trie_search(root, "w.Baidu.com");
    if (response!=NULL) printf("%s\n", response);
    else printf("none\n");

    response = trie_search(root, "123.123");
    if (response!=NULL) printf("%s\n", response);
    else printf("none\n");

    trie_delete(root, "www.google.com");

    response = trie_search(root, "www.google.com");
    if (response!=NULL) printf("%s\n", response);
    else printf("none\n");
    
    trie_delete(root, "www.baidu.com");

    response = trie_search(root, "www.baidu.com");
    if (response!=NULL) printf("%s\n", response);
    else printf("none\n");

    trie_insert(root, "www.baidu.com", "0.0.0.0");

    response = trie_search(root, "www.baidu.com");
    if (response!=NULL) printf("%s\n", response);
    else printf("none\n");

    trie_delete(root, "ww.baidu.com");

    response = trie_search(root, "ww.baidu.com");
    if (response!=NULL) printf("%s\n", response);
    else printf("none\n");

    trie_insert(root, "ww.baidu.com", "0.0.0.0");

    response = trie_search(root, "ww.baidu.com");
    if (response!=NULL) printf("%s\n", response);
    else printf("none\n");

    response = trie_search(root, "www.baidu.com");
    if (response!=NULL) printf("%s\n", response);
    else printf("none\n");

    trie_free(root);

    response = trie_search(root, "www.baidu.com");
    if (response!=NULL) printf("%s\n", response);
    else printf("none\n");

    system("pause");
    return 0;
}