#include "tcp.h"
#include "websocket.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <string>
#include <vector>
#include <memory>
#include <stdint.h>
#include <iostream>

using namespace std;

struct openssl_context
{
    SSL_CTX* ctx = nullptr;

    openssl_context()
    {
        SSL_load_error_strings();
        OpenSSL_add_ssl_algorithms();
        ctx = SSL_CTX_new(DTLS_client_method());
    }

    ~openssl_context()
    {
        EVP_cleanup();
    }
};

shared_ptr<openssl_context> shared_context()
{
    static shared_ptr<openssl_context> context;
    if (!context)
    {
        context = make_shared<openssl_context>();
    }
    return context;
}

int main()
{
    string   host = "testnet.binance.vision";
    uint16_t port = 443;
    string   uri  = "/stream?streams=btcusdt@trade&timeUnit=MICROSECOND";

    string websocket_key = "ZDjAIhP1CSBvruG9uw820A==";

    int fd = tcp_open(0, 0);
    
    int connect_code = tcp_connect(fd, host.c_str(), port);

    cout << "tcp_connect " << connect_code << endl;

    SSL* ssl = SSL_new(shared_context()->ctx);
    SSL_set_fd(ssl, fd);

    int ssl_code = SSL_connect(ssl);

    cout << "SSL_connect " << ssl_code << endl;

    string http_upgrade = websocket_upgrade(host, uri, websocket_key);

    int send_size = tcp_send(fd, (uint8_t*)http_upgrade.c_str(), http_upgrade.size());

    cout << "tcp_send " << send_size << endl;
    
    vector<uint8_t> buff(1024 * 1024);

    //while(true)
    {
        int recv_size = tcp_recv(fd, buff.data(), buff.size());

        cout << "tcp_recv " << recv_size << endl;
    }

    return 0;
}
