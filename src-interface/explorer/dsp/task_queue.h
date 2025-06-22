#pragma once

#include <exception>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace satdump
{
    class TaskQueue
    {
    private:
        std::thread task_thread;
        std::mutex queue_mtx;
        std::queue<std::function<void()>> task_queue;

        bool thread_exited = true;

    private:
        void threadFunc()
        {
        recheck:
            queue_mtx.lock();
            bool queue_has_data = task_queue.size() > 0;
            queue_mtx.unlock();

            if (queue_has_data)
            {
                queue_mtx.lock();
                auto task = task_queue.front();
                task_queue.pop();
                queue_mtx.unlock();

                try
                {
                    task();
                }
                catch (std::exception &e)
                {
                    // TODOREWORK?
                }

                goto recheck;
            }

            thread_exited = true;
        }

    public:
        TaskQueue() {}

        ~TaskQueue()
        {
            if (task_thread.joinable())
                task_thread.join();
        }

        void push(std::function<void()> task)
        {
            queue_mtx.lock();

            task_queue.push(task);

            if (thread_exited)
            {
                if (task_thread.joinable())
                    task_thread.join();
                thread_exited = false;
                task_thread = std::thread(&TaskQueue::threadFunc, this);
            }

            queue_mtx.unlock();
        }
    };
} // namespace satdump