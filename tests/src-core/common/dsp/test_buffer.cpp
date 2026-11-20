#include "common/dsp/buffer.h"
#include "common/dsp/dsp_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <future>
#include <numeric>

TEST_CASE("create_volk_buffer returns aligned zeroed memory", "[common][dsp][buffer]")
{
    for (int size : {1, 7, 64, 1000})
    {
        float *f = dsp::create_volk_buffer<float>(size);
        REQUIRE(f != nullptr);
        CHECK(((uintptr_t)f % volk_get_alignment()) == 0);
        for (int i = 0; i < size; i++)
            REQUIRE(f[i] == 0.0f);
        volk_free(f);

        complex_t *c = dsp::create_volk_buffer<complex_t>(size);
        REQUIRE(c != nullptr);
        CHECK(((uintptr_t)c % volk_get_alignment()) == 0);
        for (int i = 0; i < size; i++)
        {
            REQUIRE(c[i].real == 0.0f);
            REQUIRE(c[i].imag == 0.0f);
        }
        volk_free(c);
    }
}

TEST_CASE("Default stream and ring buffer sizes", "[common][dsp][buffer]")
{
    CHECK(dsp::STREAM_BUFFER_SIZE == 1000000);
    CHECK(dsp::RING_BUF_SZ == 1000000);
}

TEST_CASE("stream swap hands data from writer to reader", "[common][dsp][buffer]")
{
    dsp::stream<int> s(16);
    CHECK(s.getReady() == false);

    for (int i = 0; i < 10; i++)
        s.writeBuf[i] = i * 3;
    REQUIRE(s.swap(10));

    // The reader sees the data through readBuf once read() returns
    REQUIRE(s.read() == 10);
    CHECK(s.getDataSize() == 10);
    for (int i = 0; i < 10; i++)
        CHECK(s.readBuf[i] == i * 3);
    s.flush();
    CHECK(s.getReady() == false);

    // A second round reuses the swapped buffers
    for (int i = 0; i < 5; i++)
        s.writeBuf[i] = 100 + i;
    REQUIRE(s.swap(5));
    REQUIRE(s.read() == 5);
    for (int i = 0; i < 5; i++)
        CHECK(s.readBuf[i] == 100 + i);
    s.flush();
}

TEST_CASE("stream swap waits for the reader to flush", "[common][dsp][buffer]")
{
    dsp::stream<int> s(8);
    s.writeBuf[0] = 1;
    REQUIRE(s.swap(1));

    // Second swap has to wait until the first buffer was consumed
    std::atomic<bool> second_done{false};
    auto writer = std::async(std::launch::async,
                             [&]()
                             {
                                 s.writeBuf[0] = 2;
                                 bool r = s.swap(1);
                                 second_done = true;
                                 return r;
                             });

    REQUIRE(s.read() == 1);
    CHECK(s.readBuf[0] == 1);
    CHECK_FALSE(second_done.load()); // Can't have swapped, we still hold the buffer
    s.flush();

    REQUIRE(writer.wait_for(dsp_test::kTimeout) == std::future_status::ready);
    CHECK(writer.get());
    REQUIRE(s.read() == 1);
    CHECK(s.readBuf[0] == 2);
    s.flush();
}

TEST_CASE("stream stopReader makes read return -1", "[common][dsp][buffer]")
{
    dsp::stream<float> s(8);
    s.stopReader();
    CHECK(s.read() == -1);

    // Even pending data is not returned once the reader is stopped
    REQUIRE(s.swap(3));
    CHECK(s.read() == -1);

    s.clearReadStop();
    CHECK(s.read() == 3);
    s.flush();
}

TEST_CASE("stream stopReader wakes a blocked reader", "[common][dsp][buffer]")
{
    dsp::stream<float> s(8);
    auto reader = std::async(std::launch::async, [&]() { return s.read(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    s.stopReader();
    REQUIRE(reader.wait_for(dsp_test::kTimeout) == std::future_status::ready);
    CHECK(reader.get() == -1);
}

TEST_CASE("stream stopWriter aborts a pending swap", "[common][dsp][buffer]")
{
    dsp::stream<float> s(8);
    REQUIRE(s.swap(1)); // Fills the only slot, nobody reads it

    auto writer = std::async(std::launch::async, [&]() { return s.swap(1); });
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    s.stopWriter();
    REQUIRE(writer.wait_for(dsp_test::kTimeout) == std::future_status::ready);
    CHECK(writer.get() == false);

    // Stopped writer refuses immediately, until cleared
    CHECK(s.swap(1) == false);
    s.clearWriteStop();
    REQUIRE(s.read() == 1);
    s.flush();
    CHECK(s.swap(2) == true);
}

TEST_CASE("stream transfers a long sequence between threads in order", "[common][dsp][buffer]")
{
    auto s = std::make_shared<dsp::stream<int>>(1024);
    std::vector<int> data(100000);
    std::iota(data.begin(), data.end(), 0);

    dsp_test::Collector<int> col(s);
    REQUIRE(dsp_test::feedStream(s, data, 777));
    auto out = col.finish();

    REQUIRE(out == data);
}

TEST_CASE("RingBuffer write then read returns the same data", "[common][dsp][buffer][ringbuffer]")
{
    dsp::RingBuffer<int32_t> rb(1000);
    CHECK(rb.getReadable() == 0);
    CHECK(rb.getWritable() == 1000);

    std::vector<int32_t> in(600), out(600);
    std::iota(in.begin(), in.end(), 1);

    REQUIRE(rb.write(in.data(), 600) == 600);
    CHECK(rb.getReadable() == 600);
    // Limited by max latency, not by the ring size
    CHECK(rb.getWritable() == 400);

    REQUIRE(rb.read(out.data(), 600) == 600);
    CHECK(out == in);
    CHECK(rb.getReadable() == 0);
}

TEST_CASE("RingBuffer handles wrap-around", "[common][dsp][buffer][ringbuffer]")
{
    dsp::RingBuffer<int32_t> rb(dsp::RING_BUF_SZ);
    const int chunk = dsp::RING_BUF_SZ * 3 / 5;

    std::vector<int32_t> in(chunk), out(chunk);
    for (int round = 0; round < 3; round++)
    {
        for (int i = 0; i < chunk; i++)
            in[i] = round * chunk + i;
        REQUIRE(rb.write(in.data(), chunk) == chunk);
        REQUIRE(rb.read(out.data(), chunk) == chunk);
        REQUIRE(out == in);
    }
}

TEST_CASE("RingBuffer readAndSkip drops the requested samples", "[common][dsp][buffer][ringbuffer]")
{
    dsp::RingBuffer<int32_t> rb(1000);
    std::vector<int32_t> in(30);
    std::iota(in.begin(), in.end(), 0);
    REQUIRE(rb.write(in.data(), 30) == 30);

    std::vector<int32_t> out(10);
    REQUIRE(rb.readAndSkip(out.data(), 10, 5) == 10);
    for (int i = 0; i < 10; i++)
        CHECK(out[i] == i);

    // 5 samples skipped, next read starts at 15
    REQUIRE(rb.read(out.data(), 10) == 10);
    for (int i = 0; i < 10; i++)
        CHECK(out[i] == 15 + i);
    CHECK(rb.getReadable() == 5);
}

TEST_CASE("RingBuffer stop flags make read and write fail", "[common][dsp][buffer][ringbuffer]")
{
    dsp::RingBuffer<float> rb(100);
    std::vector<float> buf(10, 1.0f);

    rb.stopReader();
    CHECK(rb.getReadStop());
    CHECK(rb.read(buf.data(), 10) == -1);
    rb.clearReadStop();
    CHECK_FALSE(rb.getReadStop());

    rb.stopWriter();
    CHECK(rb.getWriteStop());
    CHECK(rb.write(buf.data(), 10) == -1);
    rb.clearWriteStop();
    CHECK(rb.write(buf.data(), 10) == 10);
    CHECK(rb.read(buf.data(), 10) == 10);
}

TEST_CASE("RingBuffer setMaxLatency limits writable space", "[common][dsp][buffer][ringbuffer]")
{
    dsp::RingBuffer<float> rb(100);
    std::vector<float> buf(50, 0.0f);
    REQUIRE(rb.write(buf.data(), 50) == 50);
    CHECK(rb.getWritable() == 50);

    rb.setMaxLatency(60);
    CHECK(rb.getWritable() == 10);

    rb.setMaxLatency(40); // Already over latency, never negative
    CHECK(rb.getWritable() == 0);
}

TEST_CASE("RingBuffer streams data between a producer and a consumer", "[common][dsp][buffer][ringbuffer]")
{
    dsp::RingBuffer<int32_t> rb(4096);
    const int total = 200000;

    auto producer = std::async(std::launch::async,
                               [&]()
                               {
                                   std::vector<int32_t> buf(1000);
                                   for (int pos = 0; pos < total; pos += 1000)
                                   {
                                       std::iota(buf.begin(), buf.end(), pos);
                                       if (rb.write(buf.data(), 1000) != 1000)
                                           return false;
                                   }
                                   return true;
                               });

    std::vector<int32_t> out(total);
    for (int pos = 0; pos < total; pos += 1500)
    {
        int n = std::min(1500, total - pos);
        REQUIRE(rb.read(&out[pos], n) == n);
    }

    REQUIRE(producer.wait_for(dsp_test::kTimeout) == std::future_status::ready);
    CHECK(producer.get());
    size_t mismatches = 0, first = 0;
    for (int i = 0; i < total; i++)
        if (!(out[i] == i) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}
