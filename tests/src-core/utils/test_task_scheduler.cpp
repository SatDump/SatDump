#include "core/plugin.h"
#include "utils/task_scheduler.h"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

// TaskScheduler fires due tasks through the global satdump::eventBus. EventBus has no way to remove
// handlers, so each test registers a handler for an event type private to this file; the handler
// only holds a shared_ptr to its recorder, so it stays harmless once the test is over.
//
// Every test uses its own TaskScheduler instance, never the global satdump::taskScheduler.

namespace
{
    // Counts events per tag and lets a test wait for them
    struct Recorder
    {
        std::mutex mtx;
        std::condition_variable cv;
        std::map<std::string, int> counts;
        std::map<std::string, int> last_payload;

        void hit(const std::string &tag, int payload)
        {
            {
                std::lock_guard<std::mutex> l(mtx);
                counts[tag]++;
                last_payload[tag] = payload;
            }
            cv.notify_all();
        }

        int count(const std::string &tag)
        {
            std::lock_guard<std::mutex> l(mtx);
            return counts.count(tag) ? counts[tag] : 0;
        }

        bool waitFor(const std::string &tag, int n, std::chrono::milliseconds timeout)
        {
            std::unique_lock<std::mutex> l(mtx);
            return cv.wait_for(l, timeout, [&] { return counts.count(tag) && counts[tag] >= n; });
        }
    };

    struct SchedEventA
    {
        int payload;
    };
    struct SchedEventB
    {
        int payload;
    };
    struct SchedEventC
    {
        int payload;
    };
    struct SchedEventD
    {
        int payload;
    };
    struct SchedEventE
    {
        int payload;
    };
    struct SchedEventF
    {
        int payload;
    };
    struct SchedEventG
    {
        int payload;
    };
    struct SchedEventH
    {
        int payload;
    };
    struct SchedEventI
    {
        int payload;
    };

    template <typename T>
    std::shared_ptr<Recorder> listen(const std::string &tag)
    {
        auto rec = std::make_shared<Recorder>();
        satdump::eventBus->register_handler<T>([rec, tag](T e) { rec->hit(tag, e.payload); });
        return rec;
    }

    const auto LONG_TIMEOUT = std::chrono::milliseconds(20000);

    // ~TaskScheduler notifies without holding the mutex, so a notify sent right as the thread starts
    // waiting can be lost (see UNTESTED_ISSUES.md). Letting the thread settle into its wait first avoids that.
    void settle() { std::this_thread::sleep_for(std::chrono::milliseconds(150)); }
} // namespace

TEST_CASE("TaskScheduler fires a due task once and reschedules it", "[utils][task_scheduler]")
{
    auto rec = listen<SchedEventA>("a");
    {
        satdump::TaskScheduler sched;
        // last_run 0 means the task is long overdue; the hour long interval keeps it from repeating
        sched.add_task("unit_test_a", std::make_shared<SchedEventA>(SchedEventA{1234}), 0, 3600);
        sched.start_thread();

        REQUIRE(rec->waitFor("a", 1, LONG_TIMEOUT));
        settle();
        CHECK(rec->count("a") == 1);
        CHECK(rec->last_payload["a"] == 1234);
    }
}

TEST_CASE("TaskScheduler repeats a short interval task", "[utils][task_scheduler]")
{
    auto rec = listen<SchedEventB>("b");
    {
        satdump::TaskScheduler sched;
        sched.add_task("unit_test_b", std::make_shared<SchedEventB>(SchedEventB{5}), 0, 1);
        sched.start_thread();

        // Fires immediately, then about once per second
        REQUIRE(rec->waitFor("b", 2, LONG_TIMEOUT));
        settle();
    }
    CHECK(rec->count("b") >= 2);
}

TEST_CASE("TaskScheduler del_task removes a task before it runs", "[utils][task_scheduler]")
{
    auto rec_removed = listen<SchedEventC>("removed");
    auto rec_kept = listen<SchedEventD>("kept");
    {
        satdump::TaskScheduler sched;
        sched.add_task("unit_test_removed", std::make_shared<SchedEventC>(SchedEventC{1}), 0, 3600);
        sched.del_task("unit_test_removed");
        sched.del_task("unit_test_never_added"); // No-op
        sched.add_task("unit_test_kept", std::make_shared<SchedEventD>(SchedEventD{2}), 0, 3600);
        sched.start_thread();

        REQUIRE(rec_kept->waitFor("kept", 1, LONG_TIMEOUT));
        settle();
    }
    CHECK(rec_removed->count("removed") == 0);
    CHECK(rec_kept->count("kept") == 1);
}

TEST_CASE("TaskScheduler add_task with an existing name replaces the task", "[utils][task_scheduler]")
{
    auto rec_old = listen<SchedEventE>("old");
    auto rec_new = listen<SchedEventF>("new");
    {
        satdump::TaskScheduler sched;
        sched.add_task("unit_test_same_name", std::make_shared<SchedEventE>(SchedEventE{1}), 0, 3600);
        sched.add_task("unit_test_same_name", std::make_shared<SchedEventF>(SchedEventF{99}), 0, 3600);
        sched.start_thread();

        REQUIRE(rec_new->waitFor("new", 1, LONG_TIMEOUT));
        settle();
    }
    CHECK(rec_old->count("old") == 0);
    CHECK(rec_new->count("new") == 1);
    CHECK(rec_new->last_payload["new"] == 99);
}

TEST_CASE("TaskScheduler destructor without start_thread is safe", "[utils][task_scheduler]")
{
    auto rec = listen<SchedEventH>("unstarted");
    {
        satdump::TaskScheduler sched;
        sched.add_task("unit_test_unstarted", std::make_shared<SchedEventH>(SchedEventH{0}), 0, 1);
    }
    CHECK(rec->count("unstarted") == 0);
}

// Bug: task_scheduler.cpp:30 compares against `wait_began - next_run` instead of
// `next_run - wait_began`. That comparison is always true for future tasks, so the scheduler
// sleeps until the *last* future task in map order rather than the earliest one. Here the task
// due in ~1 s is shadowed by one due in an hour and never fires.
TEST_CASE("TaskScheduler runs the earliest future task when several are pending", "[utils][task_scheduler][!shouldfail]")
{
    auto rec = listen<SchedEventG>("soon");
    {
        satdump::TaskScheduler sched;
        time_t now = time(nullptr);
        // Names chosen so the soon task sorts first in the std::map
        sched.add_task("unit_test_1_soon", std::make_shared<SchedEventG>(SchedEventG{1}), now - 99, 100);
        sched.add_task("unit_test_2_late", std::make_shared<SchedEventI>(SchedEventI{2}), now, 3600);
        sched.start_thread();

        CHECK(rec->waitFor("soon", 1, std::chrono::milliseconds(3000)));
        settle();
    }
}
