#include "dsp/base/readerwritercircularbuffer.h"
#include "dsp/ndsp_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <future>
#include <string>
#include <thread>

using moodycamel::BlockingReaderWriterCircularBuffer;

TEST_CASE("ndsp BlockingReaderWriterCircularBuffer respects its exact capacity", "[dsp][base][circular_buffer]")
{
    // Capacity is rounded up internally, but the exact value is enforced
    BlockingReaderWriterCircularBuffer<int> q(5);
    CHECK(q.max_capacity() == 5);
    for (int i = 0; i < 5; i++)
        CHECK(q.try_enqueue(i));
    CHECK_FALSE(q.try_enqueue(99));
    CHECK(q.size_approx() == 5);

    int v = -1;
    for (int i = 0; i < 5; i++)
    {
        REQUIRE(q.try_dequeue(v));
        CHECK(v == i);
    }
    CHECK_FALSE(q.try_dequeue(v));
    CHECK(q.size_approx() == 0);
}

TEST_CASE("ndsp BlockingReaderWriterCircularBuffer wraps around", "[dsp][base][circular_buffer]")
{
    BlockingReaderWriterCircularBuffer<int> q(3);
    int next_in = 0, next_out = 0;
    for (int round = 0; round < 10; round++)
    {
        while (q.try_enqueue(next_in))
            next_in++;
        int v;
        REQUIRE(q.try_dequeue(v));
        CHECK(v == next_out++);
        REQUIRE(q.try_dequeue(v));
        CHECK(v == next_out++);
    }
    int v;
    while (q.try_dequeue(v))
        CHECK(v == next_out++);
    CHECK(next_out == next_in);
}

TEST_CASE("ndsp BlockingReaderWriterCircularBuffer holds non-trivial types", "[dsp][base][circular_buffer]")
{
    BlockingReaderWriterCircularBuffer<std::string> q(2);
    q.wait_enqueue(std::string(1000, 'a'));
    q.wait_enqueue("second");
    std::string s;
    REQUIRE(q.try_dequeue(s));
    CHECK(s == std::string(1000, 'a'));
    q.wait_dequeue(s);
    CHECK(s == "second");
    // Leftover items are destroyed with the queue
    q.wait_enqueue("left over");
}

TEST_CASE("ndsp BlockingReaderWriterCircularBuffer timed operations time out", "[dsp][base][circular_buffer]")
{
    BlockingReaderWriterCircularBuffer<int> q(1);
    int v;
    CHECK_FALSE(q.wait_dequeue_timed(v, std::chrono::milliseconds(5)));
    CHECK(q.wait_enqueue_timed(1, std::chrono::milliseconds(5)));
    CHECK_FALSE(q.wait_enqueue_timed(2, std::chrono::milliseconds(5)));
    CHECK(q.wait_dequeue_timed(v, std::chrono::milliseconds(5)));
    CHECK(v == 1);
}

TEST_CASE("ndsp BlockingReaderWriterCircularBuffer move keeps content", "[dsp][base][circular_buffer]")
{
    BlockingReaderWriterCircularBuffer<int> a(4);
    a.wait_enqueue(7);
    a.wait_enqueue(8);
    BlockingReaderWriterCircularBuffer<int> b(std::move(a));
    CHECK(b.max_capacity() == 4);
    CHECK(b.size_approx() == 2);
    int v;
    REQUIRE(b.try_dequeue(v));
    CHECK(v == 7);

    BlockingReaderWriterCircularBuffer<int> c(1);
    c = std::move(b);
    CHECK(c.max_capacity() == 4);
    REQUIRE(c.try_dequeue(v));
    CHECK(v == 8);
}

TEST_CASE("ndsp BlockingReaderWriterCircularBuffer single producer single consumer stress", "[dsp][base][circular_buffer]")
{
    BlockingReaderWriterCircularBuffer<uint64_t> q(16);
    const uint64_t n = 200000;
    auto consumer = std::async(std::launch::async,
                               [&]()
                               {
                                   uint64_t expected = 0;
                                   bool in_order = true;
                                   for (uint64_t i = 0; i < n; i++)
                                   {
                                       uint64_t v;
                                       if (!q.wait_dequeue_timed(v, ndsp_test::kTimeout))
                                           return false;
                                       if (v != expected++)
                                           in_order = false;
                                   }
                                   return in_order;
                               });
    bool produced = true;
    for (uint64_t i = 0; i < n && produced; i++)
        produced = q.wait_enqueue_timed(i, ndsp_test::kTimeout);
    REQUIRE(produced);
    REQUIRE(consumer.wait_for(ndsp_test::kTimeout) == std::future_status::ready);
    CHECK(consumer.get());
}
