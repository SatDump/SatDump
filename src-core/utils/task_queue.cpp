#include "task_queue.h"

namespace satdump
{
    TaskQueue::TaskQueue() {}

    TaskQueue::~TaskQueue()
    {
        // Signal the thread to stop and wait for it
        {
            std::lock_guard<std::mutex> lock(queue_mtx);
            should_stop = true;
        }
        queue_cv.notify_one();
        if (task_thread.joinable())
            task_thread.join();
    }

    void TaskQueue::threadFunc()
    {
        while (true)
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queue_mtx);
                // Wait until there's a task or we should stop
                queue_cv.wait(lock, [this] { return !task_queue.empty() || should_stop; });

                if (should_stop && task_queue.empty())
                    break;

                task = std::move(task_queue.front());
                task_queue.pop();
            }

            try
            {
                task();
            }
            catch (std::exception &e)
            {
                // TODOREWORK?
            }
        }

        thread_exited = true;
    }

    void TaskQueue::push(std::function<void()> task)
    {
        bool join_old = false;
        std::thread old_thread;

        {
            std::lock_guard<std::mutex> lock(queue_mtx);
            task_queue.push(std::move(task));

            if (thread_exited.load())
            {
                if (task_thread.joinable())
                {
                    old_thread = std::move(task_thread);
                    join_old = true;
                }
                thread_exited = false;
                task_thread = std::thread(&TaskQueue::threadFunc, this);
            }
        }

        if (join_old)
        {
            old_thread.join();
        }

        queue_cv.notify_one();
    }
} // namespace satdump