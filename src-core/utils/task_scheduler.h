#pragma once

/**
 * @file task_scheduler.h
 * @brief Time-based task scheduler
 */

#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <thread>

namespace satdump
{
    /**
     * @brief Struct holding a scheduled task
     *
     * This struct is not intended to be created directly. It is used internally in satdump::TaskScheduler
     *
     * @param evt shared pointer to an EventBus event struct
     * @param evt_name the name of the struct. Must match typeid(T).name()
     * @param last_run timestamp of the last time the task was run
     * @param run_interval time, in seconds, between task runs
     */
    struct ScheduledTask
    {
        std::shared_ptr<void> evt = nullptr;
        std::string evt_name;
        time_t last_run = 0;
        time_t run_interval = 0;
    };

    /**
     * @brief SatDump's internal task scheduler
     *
     * Upon creation, this class creates a single thread that runs scheduled tasks as registered
     * by other parts of the program. Developers can add and remove scheduled tasks by adding an
     * EventBus event struct, along with the last run time and the run interval. Only one task is
     * run at a time. By design, only one instance of this class is shared throughout the program.
     *
     * @param running internal; should the task scheduler thread still be running
     * @param needs_update internal; a task has just been added/removed
     * @param scheduled_tasks a map of the scheduled task name as defined by the user,
     *        and the associated ScheduledTask struct
     * @param task_thread the std::thread that runs scheduled tasks
     * @param task_mtx a mutex to protect the scheduled_tasks map from cross-thread operation
     * @param cv a condition_variable that controls waiting for a task in the task_thread
     */
    class TaskScheduler
    {
    private:
        bool running = true;
        bool needs_update = false;
        std::map<std::string, ScheduledTask> scheduled_tasks;
        std::thread task_thread;
        std::mutex task_mtx;
        std::condition_variable cv;

        /**
         * @brief The function to run within task_thread
         */
        void thread_func();

    public:
        TaskScheduler();
        ~TaskScheduler();

        /**
         * @brief Add a new scheduled task
         *
         * If a task is added with a name that already exists, the old task will be overwritten.
         *
         * @param task_name the name of the scheduled task. Arbitrary, but must be used to delete
         *        the task later
         * @param evt a shared pointer to the EventBus event struct you want to schedule
         * @param last_run timestamp of the last time the task was run
         * @param run_interval time, in seconds, between task runs
         */
        template <typename T>
        void add_task(std::string task_name, std::shared_ptr<T> evt, time_t last_run, time_t run_interval)
        {
            // Add Task
            {
                std::lock_guard<std::mutex> lock(task_mtx);
                scheduled_tasks[task_name] = {evt, typeid(T).name(), last_run, run_interval};
                needs_update = true;
            }

            // Notify thread
            cv.notify_one();
        }

        /**
         * @brief Remove a scheduled task. Does nothing if no task by the given name is registered.
         *
         * @param task_name the name of the scheduled task. Arbitrary, but must match the name used
         *        to add the task
         */
        void del_task(std::string task_name)
        {
            // Delete Task
            {
                std::lock_guard<std::mutex> lock(task_mtx);
                scheduled_tasks.erase(task_name);
                needs_update = true;
            }

            // Notify thread
            cv.notify_one();
        }
    };
} // namespace satdump