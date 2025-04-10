#include "tcp.h"
#include "log.h"
#include "json.h"
#include "task.h"
#include "endian.h"
#include "websocket.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <sched.h>
#include <unistd.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>

#include <time.h>
#include <fcntl.h>
#include <netdb.h>
#include <limits.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <stdint.h>
#include <iostream>
#include <fstream>

using namespace std;
using namespace std::chrono;
using namespace std::filesystem;

static const uint8_t websocket_opcode_continue = 0;
static const uint8_t websocket_opcode_text     = 1;
static const uint8_t websocket_opcode_binary   = 2;
static const uint8_t websocket_opcode_close    = 8;
static const uint8_t websocket_opcode_ping     = 9;
static const uint8_t websocket_opcode_pong     = 10;

struct websocket
{
    bool mask = false;
    bool sized = false;
    bool finish = false;
    uint32_t key = 0;
    uint32_t size = 0;
    string close_reason;
    uint8_t opcode = 0xff;
    vector<uint8_t> cache;
    vector<uint8_t> frame;
    uint8_t frames_opcode = 0;
    vector<uint8_t> frames;
};

void websocket_on_recv_frame(int fd, uint8_t* frame, uint32_t size, bool binary)
{
    auto tp = system_clock::now();
    auto ms = duration_cast<microseconds>(tp.time_since_epoch()).count();

    if (!binary)
    {
        string text((char*)frame, size);
        json object = json::parse(text);

        uint64_t id = (uint64_t)object["data"]["u"].number_value();
        
        string line = "updateId=" + to_string(id)  + "    cpp_recvTimeUs=" + to_string(ms);

        log(line);
    }
}

void websocket_mask(uint32_t key, uint8_t* bytes, uint32_t size)
{
    if (key != 0)
    {
        for(uint32_t i = 0; i < size; ++i)
        {
            uint32_t index = i % sizeof(key);
            uint8_t* p = (uint8_t*)&key;
            uint8_t mask = *(p + index);
            bytes[i] = bytes[i] ^ mask;
        }
    }
}

bool websocket_cache(uint32_t bytes, uint8_t* to, uint8_t* from, uint32_t& offset, uint32_t size, vector<uint8_t>& cache)
{
    uint32_t from_size = size - offset;

    if (cache.empty())
    {
        if (from_size >= bytes)
        {
            memcpy(to, from + offset, bytes);
            offset += bytes;
            return true;
        }
        else
        {
            cache.resize(from_size);
            memcpy(cache.data(), from + offset, from_size);
            offset += from_size;
            return false;
        }
    }
    else
    {
        if (cache.size() + from_size >= bytes)
        {
            uint32_t read_size = bytes - cache.size();
            memcpy(to, cache.data(), cache.size());
            memcpy(to + cache.size(), from + offset, read_size);
            offset += read_size;
            cache.clear();
            return true;
        }
        else
        {
            uint32_t old_size = cache.size();
            cache.resize(old_size + from_size);
            memcpy(cache.data() + old_size, from + offset, from_size);
            offset += from_size;
            return false;
        }
    }
}

void websocket_on_tcp_recved(int fd, SSL* ssl, uint8_t* bytes, uint32_t size)
{
    static shared_ptr<websocket> d = make_shared<websocket>();

    uint32_t offset = 0;

    while(offset < size)
    {
        if (offset < size && d->opcode == 0xff && !d->sized)
        {
            uint8_t x = *(bytes + offset);
            d->finish = x & 0x80;
            d->opcode = x & 0x7f;
            offset += sizeof(uint8_t);
            //continue;
        }

        if (d->opcode != websocket_opcode_continue && d->opcode != websocket_opcode_text && d->opcode != websocket_opcode_binary && d->opcode != websocket_opcode_close && d->opcode != websocket_opcode_ping && d->opcode != websocket_opcode_pong)
        {
            d->close_reason = "websocket unknown opcode " + to_string(d->opcode);
            cout << d->close_reason << endl;
            tcp_close(fd);
            return;
        }

        if (offset < size && d->size == 0 && !d->sized)
        {
            uint8_t x = *(bytes + offset);
            d->mask = x & 0x80;
            d->size = x & 0x7f;
            d->sized = d->size <= 125;
            offset += sizeof(uint8_t);
            //continue;
        }

        if (offset < size && d->size == 126 && !d->sized)
        {
            uint16_t x = 0;
            d->sized = websocket_cache(2, (uint8_t*)&x, bytes, offset, size, d->cache);
            if (d->sized) d->size = endian16(x);
            //continue;
        }

        if (offset < size && d->size == 127 && !d->sized)
        {
            uint64_t x = 0;
            d->sized = websocket_cache(8, (uint8_t*)&x, bytes, offset, size, d->cache);
            if (d->sized) d->size = (uint32_t)endian64(x);
            //continue;
        }

        if (offset < size && d->sized && d->mask && d->key == 0)
        {
            uint32_t x = 0;
            if (websocket_cache(4, (uint8_t*)&x, bytes, offset, size, d->cache))
            {
                d->key = x;
            }
            else
            {
                //continue;
            }
        }

        if (offset < size && d->sized && d->size > 0 && d->frame.size() < d->size)
        {
            d->frame.push_back(*(bytes + offset));
            offset += sizeof(uint8_t);
        }

        if (d->sized && d->size == d->frame.size())
        {
            if (!d->finish && (d->opcode == websocket_opcode_text || d->opcode == websocket_opcode_binary))
            {
                websocket_mask(d->key, d->frame.data(), d->frame.size());
                d->frames_opcode = d->opcode;
                for(auto i : d->frame) d->frames.push_back(i);
            }

            if (d->opcode == websocket_opcode_continue)
            {
                websocket_mask(d->key, d->frame.data(), d->frame.size());
                for(auto i : d->frame) d->frames.push_back(i);
                if (d->finish)
                {
                    websocket_on_recv_frame(fd, d->frames.data(), (uint32_t)d->frames.size(), d->frames_opcode == websocket_opcode_binary);
                    d->frames_opcode = 0;
                    d->frames.clear();
                }
            }

            if (d->finish && (d->opcode == websocket_opcode_text || d->opcode == websocket_opcode_binary))
            {
                websocket_mask(d->key, d->frame.data(), d->frame.size());
                websocket_on_recv_frame(fd, d->frame.data(), (uint32_t)d->frame.size(), d->frames_opcode == websocket_opcode_binary);
            }

            if (d->opcode == websocket_opcode_ping)
            {
                if (!d->finish || d->frame.size() > 125)
                {
                    if (d->frame.size() > 125) d->close_reason = "websocket too big ping frame";
                    if (!d->finish) d->close_reason = "websocket not finish ping frame";
                    cout << d->close_reason << endl;
                    tcp_close(fd);
                    return;
                }
                websocket_mask(d->key, d->frame.data(), d->frame.size());
                //cout << "websocket ping " << d->frame.size() << endl;

                vector<uint8_t> pong = websocket_header(d->frame.size(), websocket_opcode_pong, true, d->key);
                for(auto i : d->frame) pong.push_back(i);
                SSL_write(ssl, pong.data(), pong.size());
                //cout << "websocket pong " << pong.size() << endl;
                //websocket_send(socket, d->frame.data(), d->frame.size(), websocket_opcode_pong, true);
            }

            if (d->opcode == websocket_opcode_pong)
            {
            }

            if (d->opcode == websocket_opcode_close)
            {
                if (d->opcode == websocket_opcode_close) d->close_reason = "websocket close frame";
                cout << d->close_reason << endl;
                tcp_close(fd);
                return;
            }

            d->key = 0; d->size = 0; d->opcode = 0xff; d->finish = false; d->mask = false; d->sized = false; d->frame.clear();
        }
    }
}

void set_thread_affinity(int cpuid)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpuid, &mask);
    sched_setaffinity(0, sizeof(cpu_set_t), &mask);
}

void set_socket_affinity(int cpuid, int fd)
{
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,0)
    setsockopt(fd, SOL_SOCKET, SO_INCOMING_CPU, &cpuid, sizeof(cpuid));
#endif
}

int main()
{
    daemon(1, 0);

    async_task_interval([]() {
        set_thread_affinity(1);
        log_flush();
    }, 1000ms);
    
    async_task_interval([]() {
        log_clean(6);
    }, 1h);

    // openssl s_client -connect testnet.binance.vision:443
    //string   host = "testnet.binance.vision";
    string   host = "stream.binance.com";
    uint16_t port = 443;
    string   uri  = "/stream?streams=btcusdt@bookTicker&timeUnit=MICROSECOND";

    string websocket_key = "ZDjAIhP1CSBvruG9uw820A==";

    int fd = tcp_open(0, 0);

    set_thread_affinity(0);
    set_socket_affinity(0, fd);

    
    int connect_code = tcp_connect(fd, host.c_str(), port);

    cout << "TCP_connect " << connect_code << endl;

    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);

    //SSL_CTX_set_cipher_list(ctx, "ECDHE-RSA-AES128-GCM-SHA256")

    SSL* ssl = SSL_new(ctx);

    SSL_set_tlsext_host_name(ssl, host.c_str());
    
    SSL_set_fd(ssl, fd);
    
    int ssl_code = SSL_connect(ssl);

    cout << "SSL_connect " << ssl_code << endl;

    if (ssl_code <= 0)
    {
        ERR_print_errors_fp(stdout);
    }

    const SSL_CIPHER* cipher = SSL_get_current_cipher(ssl);
    cout << "SSL_cipher: " << SSL_CIPHER_get_name(cipher) << endl;


    string http_upgrade = websocket_upgrade(host, uri, websocket_key);

    SSL_write(ssl, http_upgrade.c_str(), http_upgrade.size());
    cout << endl << http_upgrade << endl;
    

    //int busypoll = 50;
    //setsockopt(fd, SOL_SOCKET, SO_BUSY_POLL, &busypoll, sizeof(int));

    fcntl(fd, F_SETFL, O_NONBLOCK);

    vector<uint8_t> buff(1024 * 1024 * 4);

    while(true)
    {
        int recv_size = SSL_read(ssl, buff.data(), buff.size());
        if (recv_size == -1)
        {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                //usleep(0);
                //timespec delay = {0, 0};
                //nanosleep(&delay, nullptr);
                continue;
            }
        }
        if (recv_size <= 0)
        {
            cout << "SSL_read " << recv_size << endl;
            ERR_print_errors_fp(stdout);
            break;
        }
        if (recv_size > 0)
        {
            static bool upgrade = false;
            if (upgrade)
            {
                websocket_on_tcp_recved(fd, ssl, buff.data(), recv_size);
            }
            else
            {
                string http_respone((char*)buff.data(), recv_size);
                cout << http_respone << endl;
                upgrade = true;
            }
        }
    }

    tcp_close(fd);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    EVP_cleanup();

    return 0;
}
