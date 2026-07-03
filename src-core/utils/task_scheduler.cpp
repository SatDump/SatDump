#include "task_scheduler.h"
#include "core/plugin.h"
#include <chrono>
#include <limits>
#include <thread>

namespace satdump
{
    void TaskScheduler::thread_func()
    {
        while (running)
        {
            std::shared_ptr<void> evt = nullptr;
            std::string evt_name;
            std::string task_to_run_name;
            time_t sleep_for = std::numeric_limits<time_t>::max();
            time_t wait_began = time(0);

            // Lock the mutex and look for next task to run
            {
                std::unique_lock<std::mutex> lock(task_mtx);

                for (auto &this_task : scheduled_tasks)
                {
                    time_t next_run = this_task.second.last_run + this_task.second.run_interval;
                    if (next_run <= wait_began)
                    {
                        evt = this_task.second.evt;
                        evt_name = this_task.second.evt_name;
                        task_to_run_name = this_task.first;
                        sleep_for = 0;
                        break;
                    }
                    else if (sleep_for > next_run - wait_began)
                    {
                        sleep_for = next_run - wait_began;
                        evt = this_task.second.evt;
                        evt_name = this_task.second.evt_name;
                        task_to_run_name = this_task.first;
                    }
                }

                // Sleep until next task, or we are woken up
                needs_update = false;
                if (sleep_for > 0 && running && !needs_update)
                {
                    cv.wait_for(lock, std::chrono::seconds(sleep_for), [this] { return !running || needs_update; });
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // TODOREWORK. Hogs CPU otherwise...

            // Stop/Restart loop if needed
            if (!running || needs_update || evt == nullptr || wait_began + sleep_for > time(NULL))
                continue;

            // Run the task that should be due now (without holding the lock!)
            eventBus->fire_event(evt.get(), evt_name);

            // Mark it for next run
            {
                std::unique_lock<std::mutex> lock(task_mtx);
                if (scheduled_tasks.count(task_to_run_name) > 0)
                {
                    scheduled_tasks[task_to_run_name].last_run = time(NULL);
                }
            }
        }
    }

    TaskScheduler::TaskScheduler() {}

    void TaskScheduler::start_thread() { task_thread = std::thread(&TaskScheduler::thread_func, this); }

    TaskScheduler::~TaskScheduler()
    {
        running = false;
        cv.notify_one();
        if (task_thread.joinable())
            task_thread.join();
    }
} // namespace satdump