#include "websocket.h"

#include <string>

using namespace std;

string websocket_upgrade(string host, string uri, string key)
{
    string header = "GET " + uri + " HTTP/1.1\r\n";
    header += "Host: " + host + "\r\n";
    header += "Upgrade: websocket\r\n";
    header += "Connection: Upgrade\r\n";
    header += "Sec-WebSocket-Key: " + key + "\r\n";
    header += "Sec-WebSocket-Version: 13\r\n";
    header += "\r\n";
    return header;
}

vector<uint8_t> websocket_header(uint32_t size, uint8_t opcode, bool finish, uint32_t key)
{
    uint64_t u64 = (uint64_t)size;
    uint8_t* x = (uint8_t*)&u64;

    vector<uint8_t> bytes;

    opcode = finish ? opcode | 0x80 : opcode;

    bytes.push_back(opcode);

    if (size <= 125)
    {
        bytes.push_back((uint8_t)size);
    }

    if (size > 125 && size < 1024 * 64)
    {
        bytes.push_back((uint8_t)126);
        bytes.push_back(*(x + 1));
        bytes.push_back(*(x + 0));
    }

    if (size >= 1024 * 64)
    {
        bytes.push_back((uint8_t)127);
        bytes.push_back(*(x + 7));
        bytes.push_back(*(x + 6));
        bytes.push_back(*(x + 5));
        bytes.push_back(*(x + 4));
        bytes.push_back(*(x + 3));
        bytes.push_back(*(x + 2));
        bytes.push_back(*(x + 1));
        bytes.push_back(*(x + 0));
    }

    if (key != 0)
    {
        uint8_t* y = (uint8_t*)&key;
        bytes[1] |= 0x80;
        bytes.push_back(*(y + 0));
        bytes.push_back(*(y + 1));
        bytes.push_back(*(y + 2));
        bytes.push_back(*(y + 3));
    }

    return bytes;
}
