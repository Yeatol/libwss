#pragma once

#include <stdint.h>

int tcp_open(uint32_t ip, uint16_t port);

int tcp_close(int fd);

int tcp_recv(int fd, uint8_t* data, uint32_t size);

int tcp_send(int fd, uint8_t* data, uint32_t size);

int tcp_connect(int fd, uint32_t ip, uint16_t port);

int tcp_connect(int fd, const char* domain, uint16_t port);
