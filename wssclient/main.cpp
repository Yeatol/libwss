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

int main()
{
    string   host = "testnet.binance.vision";
    uint16_t port = 443;
    string   uri  = "/stream?streams=btcusdt@trade&timeUnit=MICROSECOND";

    string websocket_key = "ZDjAIhP1CSBvruG9uw820A==";

    int fd = tcp_open(0, 0);
    
    int connect_code = tcp_connect(fd, host.c_str(), port);

    cout << "tcp_connect " << connect_code << endl;

    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

	SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    
	if (ctx == nullptr)
    {
        cout << "SSL_CTX_new falied" << endl;
    }

    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, fd);

    int ssl_code = SSL_connect(ssl);
    if (ssl_code <= 0)
    {
        ERR_print_errors_fp(stderr);
        tcp_close(fd);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        EVP_cleanup();
    }

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
