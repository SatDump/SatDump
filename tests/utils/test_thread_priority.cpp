#include "utils/thread_priority.h"
#include <catch2/catch_test_macros.hpp>
#include <future>
#include <thread>

#ifndef _WIN32
#include <pthread.h>
#include <sched.h>
#endif

// These tests only ever change the priority of threads they create, never the test runner's
// own thread, as lowering a thread to SCHED_IDLE cannot always be undone without privileges.

#ifndef _WIN32
namespace
{
    // A thread that idles until released, so its scheduling can be inspected from outside
    struct ParkedThread
    {
        std::promise<void> release;
        std::thread th;
        ParkedThread()
        {
            std::shared_future<void> f = release.get_future().share();
            th = std::thread([f]() { f.wait(); });
        }
        ~ParkedThread()
        {
            release.set_value();
            th.join();
        }
        int policy()
        {
            int pol = -1;
            sched_param p;
            pthread_getschedparam(th.native_handle(), &pol, &p);
            return pol;
        }
    };
} // namespace
#endif

TEST_CASE("thread_priority_t levels are ordered", "[utils][thread_priority]")
{
    CHECK(satdump::PRIORITY_LOWEST < satdump::PRIORITY_LOW);
    CHECK(satdump::PRIORITY_LOW < satdump::PRIORITY_NORMAL);
    CHECK(satdump::PRIORITY_NORMAL < satdump::PRIORITY_HIGH);
    CHECK(satdump::PRIORITY_HIGH < satdump::PRIORITY_HIGHEST);
#ifndef _WIN32
    // SCHED_RR priorities must be in the range the kernel accepts
    CHECK(satdump::PRIORITY_LOWEST >= sched_get_priority_min(SCHED_RR));
    CHECK(satdump::PRIORITY_HIGHEST <= sched_get_priority_max(SCHED_RR));
#endif
}

#if defined(__linux__)
TEST_CASE("setLowestThreadPriority puts a thread into SCHED_IDLE", "[utils][thread_priority]")
{
    ParkedThread t;
    if (t.policy() == SCHED_IDLE)
        SKIP("The test runner already runs under SCHED_IDLE");
    satdump::setLowestThreadPriority(t.th);
    // Lowering priority needs no privileges on Linux
    CHECK(t.policy() == SCHED_IDLE);
}

TEST_CASE("setLowestThreadPriority without arguments affects only the calling thread", "[utils][thread_priority]")
{
    int policy_before = -1;
    sched_param before;
    pthread_getschedparam(pthread_self(), &policy_before, &before);

    int policy_inside = -1;
    std::thread th(
        [&]()
        {
            satdump::setLowestThreadPriority();
            sched_param p;
            pthread_getschedparam(pthread_self(), &policy_inside, &p);
        });
    th.join();
    CHECK(policy_inside == SCHED_IDLE);

    int policy_here = -1;
    sched_param p;
    pthread_getschedparam(pthread_self(), &policy_here, &p);
    CHECK(policy_here == policy_before);
}

TEST_CASE("setThreadPriority either applies SCHED_RR or leaves the thread unchanged", "[utils][thread_priority]")
{
    // Real-time scheduling needs CAP_SYS_NICE or an RTPRIO rlimit; without them the call
    // only logs an error, so both outcomes are valid here
    ParkedThread t;
    int original = t.policy();
    CHECK_NOTHROW(satdump::setThreadPriority(t.th, satdump::PRIORITY_LOW));
    int pol = -1;
    sched_param p;
    pthread_getschedparam(t.th.native_handle(), &pol, &p);
    if (pol == SCHED_RR)
        CHECK(p.sched_priority == satdump::PRIORITY_LOW);
    else
        CHECK(pol == original);
}
#endif
