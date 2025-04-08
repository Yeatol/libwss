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
    // openssl s_client -connect testnet.binance.vision:443
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
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
    SSL_CTX_set_max_proto_version(ctx, TLS1_2_VERSION);

    SSL* ssl = SSL_new(ctx);

    SSL_set_tlsext_host_name(ssl, host.c_str());
    
    SSL_set_fd(ssl, fd);
    
    int ssl_code = SSL_connect(ssl);

    cout << "SSL_connect " << ssl_code << endl;

    if (ssl_code <= 0)
    {
        cout << "errno " << errno << endl; 
        cout << "SSL_get_error " << SSL_get_error(ssl, ssl_code) << endl;
        ERR_print_errors_fp(stdout);
    }

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

    tcp_close(fd);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    EVP_cleanup();

    return 0;
}
