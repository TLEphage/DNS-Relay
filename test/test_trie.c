#include "../src/trie.h"

const uint8_t A[]={1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4};
const char web1[]="www.baidu.com";
const char web2[]="www.google.com";

int main() {
    /*
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
    */

    DNSRecord temp;
    memcpy(temp.domain,web1,sizeof(web1));
    temp.expire_time=100;
    temp.type=1;
    temp.addr.ipv4=111;
    temp.trie_next=NULL;
    temp.trie_prev=NULL;
    temp.lru_next=NULL;
    temp.lru_prev=NULL;

    DNSRecord* a=malloc(sizeof(DNSRecord));
    memcpy(a,&temp,sizeof(DNSRecord));

    temp.type=2;
    memcpy(temp.addr.ipv6,A,sizeof(temp.addr.ipv6));
    DNSRecord* b=malloc(sizeof(DNSRecord));
    memcpy(b,&temp,sizeof(DNSRecord));

    memcpy(temp.domain,web2,sizeof(web1));
    temp.type=1;
    temp.addr.ipv4=222;
    DNSRecord* c=malloc(sizeof(DNSRecord));
    memcpy(c,&temp,sizeof(DNSRecord));

    TrieNode* root = trie_create();
    trie_insert(root, a->domain, a);
    trie_insert(root, b->domain, b);
    trie_insert(root, c->domain, c);
    trie_print(root, "www.baidu.com");
    printf("done\n");

    trie_delete(root, a->domain, a);
    trie_print(root, "www.baidu.com");
    printf("done\n");

    trie_delete(root, b->domain, b);
    trie_print(root, "www.baidu.com");
    printf("done\n");

    system("pause");
    return 0;
}