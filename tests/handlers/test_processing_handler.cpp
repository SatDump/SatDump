#include "handlers/processing_handler.h"
#include "pipeline/pipeline_test_helpers.h"
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <condition_variable>

using namespace pipeline_test;

namespace
{
    class CountingProcessor : public satdump::handlers::ProcessingHandler
    {
    public:
        std::atomic<int> runs{0};
        std::atomic<bool> was_processing_inside{false};

        // Optional gate to hold do_process() until released
        std::mutex gate_mtx;
        std::condition_variable gate_cv;
        bool gate_open = true;
        std::atomic<bool> entered{false};

        ~CountingProcessor()
        {
            openGate();
            // Join here, before this class's members are destroyed
            if (async_thread.joinable())
                async_thread.join();
        }

        void closeGate()
        {
            std::lock_guard<std::mutex> l(gate_mtx);
            gate_open = false;
        }

        void openGate()
        {
            {
                std::lock_guard<std::mutex> l(gate_mtx);
                gate_open = true;
            }
            gate_cv.notify_all();
        }

        void join()
        {
            if (async_thread.joinable())
                async_thread.join();
        }

    protected:
        void do_process()
        {
            entered = true;
            was_processing_inside = get_is_processing();
            std::unique_lock<std::mutex> l(gate_mtx);
            gate_cv.wait(l, [this]() { return gate_open; });
            runs++;
        }
    };
} // namespace

TEST_CASE("ProcessingHandler::process runs synchronously and flags processing", "[handlers][processing_handler]")
{
    CountingProcessor p;
    CHECK_FALSE(p.get_is_processing());

    p.process();
    CHECK(p.runs == 1);
    CHECK(p.was_processing_inside);
    CHECK_FALSE(p.get_is_processing());

    p.process();
    CHECK(p.runs == 2);
}

TEST_CASE("ProcessingHandler::set_is_processing overrides the status", "[handlers][processing_handler]")
{
    CountingProcessor p;
    p.set_is_processing(true);
    CHECK(p.get_is_processing());
    p.set_is_processing(false);
    CHECK_FALSE(p.get_is_processing());
}

TEST_CASE("ProcessingHandler::asyncProcess runs in a background thread", "[handlers][processing_handler]")
{
    CountingProcessor p;
    p.closeGate();

    p.asyncProcess();
    REQUIRE(test_helpers::waitUntil([&]() { return p.entered.load(); }, std::chrono::seconds(30)));
    CHECK(p.get_is_processing());
    CHECK(p.runs == 0);

    p.openGate();
    p.join();
    CHECK(p.runs == 1);
    CHECK_FALSE(p.get_is_processing());

    // Can be started again once done
    p.asyncProcess();
    p.join();
    CHECK(p.runs == 2);
}

TEST_CASE("ProcessingHandler::asyncProcess ignores requests while already processing", "[handlers][processing_handler]")
{
    CountingProcessor p;
    p.closeGate();

    p.asyncProcess();
    REQUIRE(test_helpers::waitUntil([&]() { return p.entered.load(); }, std::chrono::seconds(30)));

    p.asyncProcess(); // rejected (prints a warning to stdout)

    p.openGate();
    p.join();
    CHECK(p.runs == 1);
}
