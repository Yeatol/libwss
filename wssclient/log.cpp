#include "log.h"

#include <regex>
#include <mutex>
#include <deque>
#include <vector>
#include <string>
#include <chrono>
#include <stdio.h>
#include <fstream>
#include <algorithm>
#include <filesystem>

using namespace std;
using namespace std::chrono;
using namespace std::filesystem;

static mutex         log_lock;
static string        log_lines;
static deque<string> log_tails;

string log_time()
{
    auto tp = system_clock::now();
    auto ms = duration_cast<microseconds>(tp.time_since_epoch()).count() % 1000;
    auto tm = system_clock::to_time_t(tp);
    struct tm* d = localtime(&tm);
    vector<char> bytes(128);
    sprintf(bytes.data(), "%04d-%02d-%02d %02d:%02d:%02d.%03d", (uint32_t)d->tm_year + 1900, (uint32_t)d->tm_mon + 1, (uint32_t)d->tm_mday, (uint32_t)d->tm_hour, (uint32_t)d->tm_min, (uint32_t)d->tm_sec, (uint32_t)ms);
    return string(bytes.data());
}

void log_flush(string filename)
{
    lock_guard<mutex> guard(log_lock);
    if (!log_lines.empty())
    {
        auto tp = system_clock::now();
        auto tm = system_clock::to_time_t(tp);
        struct tm* d = localtime(&tm);
        vector<char> buff(1024);
        error_code ec;
        string path = "%04d%02d%02d%02d.log";
        sprintf(buff.data(), path.c_str(), (int)d->tm_year + 1900, (int)d->tm_mon + 1, (int)d->tm_mday, (int)d->tm_hour);
        if (filename.empty()) filename = string(buff.data(), buff.size());
        ofstream fs(filename.c_str(), ios::app);
        if (fs.is_open())
        {
            fs.write(log_lines.data(), log_lines.size());
            fs.close();
            log_lines.clear();
        }
    }
}

void log_clean(uint32_t last)
{
    string path = ".";
    regex  file_suffix("(.*)(.log)");
    vector<string> files;

    for (auto& itor : directory_iterator(path))
    {
    	auto filepath = itor.path();
    	auto filename = filepath.filename();

	    if (std::regex_match(filename.string(), file_suffix))
	    {
            files.push_back(filepath.string());
	    }
    }
    
    sort(files.begin(), files.end());

    for (size_t i = 0; files.size() > last && i < files.size() && i < files.size() - last; ++i)
    {
        remove(files[i]);
    }
}

string log_tail()
{
    lock_guard<mutex> guard(log_lock);
    string lines;
    for(auto& line : log_tails) lines += line;
    return lines;
}

void log(string line)
{
    lock_guard<mutex> guard(log_lock);
    line = log_time() + "|" + line + "\r\n";
    log_tails.push_back(line);
    if (log_tails.size() > 89) log_tails.pop_front();
    log_lines += line;
}
