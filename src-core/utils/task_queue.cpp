#include "task_queue.h"

namespace satdump
{
    TaskQueue::TaskQueue() {}

    TaskQueue::~TaskQueue()
    {
        if (task_thread.joinable())
            task_thread.join();
    }

    void TaskQueue::threadFunc()
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

    void TaskQueue::push(std::function<void()> task)
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
} // namespace satdump