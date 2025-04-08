#pragma once

#include <string>
#include <vector>
#include <stdint.h>

using namespace std;

string websocket_upgrade(string host, string uri, string key);

vector<uint8_t> websocket_header(uint32_t size, uint8_t opcode, bool finish, uint32_t key);
