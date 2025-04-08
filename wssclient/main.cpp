#include "tcp.h"
#include "websocket.h"

#include <wolfssl/ssl.h>

#include <string>
#include <vector>
#include <memory>
#include <stdint.h>
#include <iostream>

using std::endl;
using std::cout;

int main()
{
    string   host = "testnet.binance.vision";
    uint16_t port = 443;
    string   uri  = "/stream?streams=btcusdt@trade&timeUnit=MICROSECOND";

    string websocket_key = "ZDjAIhP1CSBvruG9uw820A==";

    int fd = tcp_open(0, 0);
    
    int connect_code = tcp_connect(fd, host.c_str(), port);

    cout << "tcp_connect " << connect_code << endl;

    wolfSSL_Init();
    WOLFSSL_CTX* ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
    WOLFSSL* ssl = wolfSSL_new(ctx);
    wolfSSL_set_fd(ssl, fd);
    int ssl_code = wolfSSL_connect(ssl);

    cout << "wolfSSL_connect " << ssl_code << endl;

    string http_upgrade = websocket_upgrade(host, uri, websocket_key);

    int send_size = tcp_send(fd, (uint8_t*)http_upgrade.c_str(), http_upgrade.size());

    cout << "tcp_send " << send_size << endl;
    
    vector<uint8_t> buff(1024 * 1024);

    //while(true)
    {
        int recv_size = tcp_recv(fd, buff.data(), buff.size());

        cout << "tcp_recv " << recv_size << endl;

        string respone((char*)buff.data(), recv_size);

        cout << respone << endl;
    }

    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    return 0;
}
