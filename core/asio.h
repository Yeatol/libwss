#pragma once
#include "asio.hpp"

#include <string>
#include <chrono>
#include <thread>
#include <random>
#include <stdint.h>
#include <functional>

using namespace std;

using namespace std::chrono;

using    udp = shared_ptr<asio::ip::udp::socket>;

void     async_task(function<void()> task);

void     async_timer(function<void()> task, milliseconds interval);

uint32_t ip_from_string(string ip);

string   ip_to_string(uint32_t ip);

uint32_t udp_get_host_ip();

uint16_t udp_get_host_port(udp socket);

udp      udp_open(function<void(udp socket, uint32_t ip, uint16_t port, char* data, uint32_t size)> recved, uint16_t port);

bool     udp_connect(udp socket, uint32_t ip, uint16_t port);

void     udp_close(udp socket);

uint32_t udp_send(udp socket, const void* data, uint32_t size);

uint32_t udp_send(udp socket, uint32_t ip, uint16_t port, const void* data, uint32_t size);

uint32_t udp_send(udp socket, uint32_t ip, uint16_t port, const void* head, uint32_t head_size, const void* body, uint32_t body_size);

void     udp_set_ttl(udp socket, int ttl);

int tcp_open(uint32_t ip, uint16_t port);

int tcp_close(int fd);

int tcp_recv(int fd, uint8_t* data, uint32_t size);

int tcp_send(int fd, uint8_t* data, uint32_t size);

int tcp_connect(int fd, uint32_t ip, uint16_t port);

int tcp_connect(int fd, const char* domain, uint16_t port);