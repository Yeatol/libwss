#pragma once

#include <chrono>
#include <functional>

using namespace std;

using namespace std::chrono;

void async_pool(function<void()> task);

void async_idle(function<void()> task);

void async_task(function<void()> task);

void async_task_timing(function<void()> task, string match);

void async_task_delay(function<void()> task, milliseconds delay);

void async_task_interval(function<void()> task, milliseconds interval);