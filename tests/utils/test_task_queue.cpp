#include "utils/task_queue.h"
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <future>
#include <stdexcept>
#include <thread>
#include <vector>

// Every test holds back the worker with a gate task until all tasks are pushed. TaskQueue::push only
// starts a worker when the previous one has exited, and the exit flag is set after the queue lock is
// released (see the race noted in the report), so pushing while the worker drains could strand a task.

namespace
{
    const auto TIMEOUT = std::chrono::seconds(30);
}

TEST_CASE("TaskQueue runs tasks sequentially in push order", "[utils][task_queue]")
{
    std::vector<int> order;
    std::promise<void> gate;
    std::shared_future<void> gate_f = gate.get_future().share();
    std::thread::id main_id = std::this_thread::get_id();
    std::vector<std::thread::id> worker_ids;

    {
        satdump::TaskQueue q;
        q.push([gate_f]() { gate_f.wait(); });
        for (int i = 0; i < 100; i++)
            q.push(
                [&order, &worker_ids, i]()
                {
                    order.push_back(i);
                    worker_ids.push_back(std::this_thread::get_id());
                });
        gate.set_value();
        // Destructor waits for the queue to drain
    }

    REQUIRE(order.size() == 100);
    for (int i = 0; i < 100; i++)
        CHECK(order[i] == i);
    // All tasks ran on one thread, which is not the caller
    for (auto &id : worker_ids)
    {
        CHECK(id == worker_ids[0]);
        CHECK(id != main_id);
    }
}

TEST_CASE("TaskQueue keeps running after a task throws", "[utils][task_queue]")
{
    std::vector<int> order;
    std::promise<void> gate;
    std::shared_future<void> gate_f = gate.get_future().share();

    {
        satdump::TaskQueue q;
        q.push([gate_f]() { gate_f.wait(); });
        q.push([&]() { order.push_back(1); });
        q.push([]() { throw std::runtime_error("task failure"); });
        q.push([&]() { order.push_back(2); });
        gate.set_value();
    }

    CHECK(order == std::vector<int>{1, 2});
}

TEST_CASE("TaskQueue restarts its worker for tasks pushed after it went idle", "[utils][task_queue]")
{
    std::atomic<int> runs{0};
    satdump::TaskQueue q;

    std::promise<void> first_done;
    q.push(
        [&]()
        {
            runs++;
            first_done.set_value();
        });
    REQUIRE(first_done.get_future().wait_for(TIMEOUT) == std::future_status::ready);

    // Give the worker ample time to notice the empty queue and exit
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::promise<void> second_done;
    q.push(
        [&]()
        {
            runs++;
            second_done.set_value();
        });
    REQUIRE(second_done.get_future().wait_for(TIMEOUT) == std::future_status::ready);
    CHECK(runs == 2);
}

TEST_CASE("TaskQueue destructor with nothing pushed returns immediately", "[utils][task_queue]")
{
    CHECK_NOTHROW([]() { satdump::TaskQueue q; }());
}
