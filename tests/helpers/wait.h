#pragma once

/*
Bounded waiting, shared by the whole suite. Everything is inline and lives in
the test_helpers namespace, as every test file is linked into the same
executable. Every blocking wait in a test is bounded, so a hang fails the
test instead of stalling the run.
*/

#include <chrono>
#include <functional>
#include <thread>

namespace test_helpers
{
    // Generous, as other heavy jobs may run on the machine at the same time
    constexpr std::chrono::milliseconds kTimeout(20000);

    // Polls pred until it returns true (returns true) or timeout expires (returns false)
    inline bool waitUntil(const std::function<bool()> &pred, std::chrono::milliseconds timeout = kTimeout, std::chrono::microseconds poll = std::chrono::microseconds(200))
    {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        while (!pred())
        {
            if (std::chrono::steady_clock::now() > deadline)
                return false;
            std::this_thread::sleep_for(poll);
        }
        return true;
    }
} // namespace test_helpers
