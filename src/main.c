#include "server.h"
#include "dnsStruct.h"


int main() {


    init();

    poll();

    closesocket(sock);

    return 0;
}