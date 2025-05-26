#pragma once

/**
 * @file thread_priority.h
 */

#include <thread>

namespace satdump
{
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

    /**
     * @brief Set specified thread priority
     * on the specified thread.
     *
     * @param th the thread to apply to
     * @param priority priority to set
     */
    void setThreadPriority(std::thread &th, thread_priority_t priority);

    /**
     * @brief Set lowest possible thread priority
     * on the specified thread.
     *
     * @param th the thread to apply to
     */
    void setLowestThreadPriority(std::thread &th);

    /**
     * @brief Set lowest possible thread priority
     * on the current thread.
     */
    void setLowestThreadPriority();
} // namespace satdump