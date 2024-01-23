#pragma once

#include <thread>

enum thread_priority_t
{
#ifdef _WIN32
    PRIORITY_LOWEST = -2,
    PRIORITY_LOW = -1,
    PRIORITY_NORMAL = 0,
    PRIORITY_HIGH = 1,
    PRIORITY_HIGHEST = 2,
#else
    PRIORITY_LOWEST = 1,
    PRIORITY_LOW = 24,
    PRIORITY_NORMAL = 50,
    PRIORITY_HIGH = 74,
    PRIORITY_HIGHEST = 99,
#endif
};

void setThreadPriority(std::thread &th, thread_priority_t priority);
void setLowestThreadPriority(std::thread &th);
void setLowestThreadPriority();