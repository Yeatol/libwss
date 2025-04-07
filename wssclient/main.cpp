#include "tcp.h"
#include "http.h"
#include "websocket.h"

#include <string>
#include <vector>
#include <stdint.h>
#include <iostream>

using namespace std;

int main()
{
    string   host = "stream.binance.com";
    uint16_t port = 443;
    string   uri  = "/stream?streams=btcusdt@trade&timeUnit=MICROSECOND";

    string websocket_key = "";

    int fd = tcp_open(0, 0);
    
    tcp_connect(fd, host.c_str(), port);

    string http_upgrade = websocket_upgrade(host, uri, websocket_key);

    tcp_send(fd, http_upgrade.c_str(), http_upgrade.size());
    
    vector<uint8_t> buff(1024 * 1024);

    int size = tcp_recv(fd, buff.data(), buff.size());

    cout << size << endl;

    return 0;
}
