#include "tcp.h"
#include "http.h"
#include "websocket.h"

#include <string>
#include <stdint.h>
#include <iostream>

using namespace std;

int main()
{
    int fd = tcp_open(0, 0);
    tcp_connect(fd, "qq.com", 80);
    tcp_send(fd, );
    return 0;
}
