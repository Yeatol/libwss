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
