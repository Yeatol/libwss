#include "http.h"

#include <string>

using namespace std;

string http_get(string host, string uri)
{
    string header = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: close\r\n\r\n";

    string header = "GET " + uri + " HTTP/1.1\r\n";
    header += "Host: " + host + "\r\n";
    header += "Connection: \r\n";
    header += "\r\n";
    return header;
}
