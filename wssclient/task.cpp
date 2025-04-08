#include "task.h"

#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <chrono>
#include <functional>
#include <condition_variable>

using namespace std;

using namespace std::chrono;

using abs_time_t = system_clock::time_point;

static abs_time_t abs_now()
{
    return system_clock::now();
}

static time_t to_time(system_clock::time_point time)
{
    return system_clock::to_time_t(time);
}

static string to_string(abs_time_t abs_time)
{
    time_t tm = to_time(abs_time);
    string time(22, 0);
    strftime((char*)time.data(), time.size(), "%Y-%m-%d %H:%M:%S %w", localtime(&tm));
    return time;
}

static bool match_time(string time, string filter)
{
    for (size_t i = 0; i < filter.size(); ++i)
    {
        if (filter[i] == '*') continue;
        if (time[i] != filter[i]) return false;
    }
    return true;
}

struct TaskThread
{
    size_t thread_count = 1;
    condition_variable_any cv;
    recursive_mutex recursive_lock;
    multimap<abs_time_t, function<int64_t(abs_time_t)>> tasks;

    TaskThread(size_t count = 1)
    {
        thread_count = count;
    }

    void async(function<int64_t(abs_time_t)> task, abs_time_t timing = abs_now())
    {
        unique_lock<recursive_mutex>lock(recursive_lock);
        static once_flag runonce;
        call_once(runonce, [=]() {
            for (size_t i = 0; i < thread_count; ++i)
            {
                thread([this]() { loop(); }).detach();
            }
        });
        tasks.emplace(timing, task);
        cv.notify_one();
    }

    void loop()
    {
        while (true)
        {
            unique_lock<recursive_mutex>lock(recursive_lock);
            while (tasks.empty())
            {
                cv.wait(lock);
            }
            if (tasks.begin()->first <= abs_now() || (cv.wait_until(lock, tasks.begin()->first) == cv_status::timeout && !tasks.empty()))
            {
                auto itor = tasks.begin();
                auto time = itor->first;
                auto task = itor->second;
                int64_t interval = task(time);
                tasks.erase(itor);
                if (interval >= 0)
                {
                    tasks.emplace(time + milliseconds(interval), task);
                }
            }
        }
    }
};

static TaskThread task_thread;

void async_pool(function<void()> task)
{
    static TaskThread calc_thread(thread::hardware_concurrency());
    calc_thread.async([=](abs_time_t) { task(); return -1; });
}

void async_idle(function<void()> task)
{
    static TaskThread idle_thread;
    idle_thread.async([=](abs_time_t) { task(); return -1; });
}

void async_task(function<void()> task)
{
    task_thread.async([=](abs_time_t) { task(); return -1; });
}

void async_task_delay(function<void()> task, milliseconds delay)
{
    task_thread.async([=](abs_time_t) { task(); return -1; }, abs_now() + delay);
}

void async_task_interval(function<void()> task, milliseconds interval)
{
    if (interval.count() < 50) interval = milliseconds(50);
    task_thread.async([=](abs_time_t time) { if (abs_now() - time <= milliseconds(50)) task(); return interval.count(); });
}

void async_task_timing(function<void()> task, string match)
{
    static mutex lock_;
    static map<string, function<void()>> matchs;
    lock_guard<mutex> lock(lock_);
    if (matchs.empty())
    {
        task_thread.async([](abs_time_t abs_time) {
            lock_guard<mutex> lock(lock_);
            string time = to_string(abs_time);
            for (auto& i : matchs)
            {
                if (match_time(time, i.first)) i.second();
            }
            return 1000;
        });
    }
    matchs.emplace(match, task);
}
