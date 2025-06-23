#include "server.h"

int main() {


    init();

    poll();

    closesocket(sock);

    return 0;
}