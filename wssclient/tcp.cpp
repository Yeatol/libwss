#include "tcp.h"

#include <netdb.h>
#include <unistd.h>
#include <limits.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <stdint.h>

using namespace std;

int tcp_open(uint32_t ip, uint16_t port)
{
    //int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (fd == -1) return -1;
    int nodelay   = 1; int reuseaddr = 1; int reuseport = 1;  int rcvbuf = 1024 * 1024 * 4; int rcvlowat = 1;
    int keepalive = 1; int keepcnt   = 3; int keepidle  = 60; int keepintvl = 30;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF,     &rcvbuf,    sizeof(int)) == -1 ||
        setsockopt(fd, SOL_SOCKET, SO_RCVLOWAT,   &rcvlowat,  sizeof(int)) == -1 ||
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,  &reuseaddr, sizeof(int)) == -1 ||
        setsockopt(fd, SOL_SOCKET, SO_REUSEPORT,  &reuseport, sizeof(int)) == -1 ||
        setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,  &keepalive, sizeof(int)) == -1 ||
        setsockopt(fd, SOL_TCP,    TCP_KEEPCNT,   &keepcnt,   sizeof(int)) == -1 ||
        setsockopt(fd, SOL_TCP,    TCP_KEEPIDLE,  &keepidle,  sizeof(int)) == -1 ||
        setsockopt(fd, SOL_TCP,    TCP_KEEPINTVL, &keepintvl, sizeof(int)) == -1 ||
        setsockopt(fd, SOL_TCP,    TCP_NODELAY,   &nodelay,   sizeof(int)) == -1)
    {
        close(fd);
        return -1;
    }
    sockaddr_in address = { 0 };
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(ip);
    address.sin_port = htons(port);
    if (bind(fd, (sockaddr*)&address, sizeof(sockaddr_in)) == -1)
    {
        close(fd);
        return -1;
    }
    return fd;
}

int tcp_close(int fd)
{
    return close(fd);
}

int tcp_recv(int fd, uint8_t* data, uint32_t size)
{
    return recv(fd, data, size, 0);
}

int tcp_send(int fd, uint8_t* data, uint32_t size)
{
    return send(fd, data, size, 0);
}

int tcp_connect(int fd, uint32_t ip, uint16_t port)
{
    sockaddr_in address = { 0 };
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(ip);
    address.sin_port = htons(port);
    return connect(fd, (sockaddr*)&address, sizeof(sockaddr_in));
}

int tcp_connect(int fd, const char* domain, uint16_t port)
{
    uint32_t ip = 0;
    addrinfo hints = { 0 };
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo *res = nullptr;

    if (getaddrinfo(domain, nullptr, &hints, &res) != 0)
    {
        return -1;
    }

    for (addrinfo* p = res; p != nullptr; p = p->ai_next)
    {
        if (p->ai_family == AF_INET)
        {
            sockaddr_in* ipv4 = (sockaddr_in*)p->ai_addr;
            ip = htonl(ipv4->sin_addr.s_addr);
            break;
        }
    }

    freeaddrinfo(res);

    return tcp_connect(fd, ip, port);
}
