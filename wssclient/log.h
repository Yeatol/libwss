#pragma once

#include <string>

using namespace std;

string log_time();

void   log_flush();

string log_tail();

void   log(string line);

void   log_clean(uint32_t last);
