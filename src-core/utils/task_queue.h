#pragma once

/**
 * @file task_queue.h
 * @brief A simple single-thread thread pool.
 */

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace satdump
{
    /**
     * @brief A simple thread pool running tasks sequentially
     * in a single thread.
     *
     * This was mostly intended to act as a much more lightweight
     * replacement for ctp_stl, as keeping threads lingering is not
     * really that useful to simply run some small background jobs.
     */
    class TaskQueue
    {
    private:
        std::thread task_thread;
        std::mutex queue_mtx;
        std::condition_variable queue_cv;
        std::queue<std::function<void()>> task_queue;

        std::atomic<bool> thread_exited{true};
        bool should_stop = false;

    private:
        void threadFunc();

    public:
        /**
         * @brief Constructor, starts the internal thread
         */
        TaskQueue();

        /**
         * @brief Destructor. Waits for remaining tasks
         * before exiting.
         */
        ~TaskQueue();

        /**
         * @brief Push a task in the queue.
         * @param task the function to push
         */
        void push(std::function<void()> task);
    };
} // namespace satdump