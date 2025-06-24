/*
gcc test\test_cache.c src\cache.c src\trie.c -o test\test_cache.exe
*/

#include "..\src\cache.h"

int main()
{
    uint32_t ipv4 = 123;
    uint8_t ipv6[16]={0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01};
    DNSCache* dns_cache = cache_create(2);
    cache_update(dns_cache, "abc.abc", RR_A, &ipv4, 100);
    cache_update(dns_cache, "bc.abc", RR_AAAA, ipv6, 100);
    trie_print(dns_cache->root, "abc.abc");
    printf("done\n");

    cache_update(dns_cache, "c.abc", RR_A, &ipv4, 100);
    trie_print(dns_cache->root, "abc.abc");
    printf("done\n");

    cache_update(dns_cache, "c.abc", RR_AAAA, ipv6, 100);
    trie_print(dns_cache->root, "bc.abc");
    printf("done\n");

    trie_print(dns_cache->root, "c.abc");
    printf("done\n");

    cache_update(dns_cache, "abc.abc", RR_A, &ipv4, 100);
    cache_update(dns_cache, "bc.abc", RR_AAAA, ipv6, 100);
    cache_update(dns_cache, "abc.abc", RR_A, &ipv4, 100);
    cache_update(dns_cache, "c.abc", RR_AAAA, ipv6, 100);

    trie_print(dns_cache->root, "abc.abc");
    printf("done\n");

    system("pause");
    return 0;
}