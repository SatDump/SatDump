#include "common/codings/viterbi/viterbi_buffer.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

TEST_CASE("ViterbiSlidingBuffer add() appends data", "[common][codings][viterbi]")
{
    viterbi::ViterbiSlidingBuffer buf(1024);
    CHECK(buf.in_buffer == 0);

    std::vector<uint8_t> a(100), b(50);
    for (int i = 0; i < 100; i++)
        a[i] = i;
    for (int i = 0; i < 50; i++)
        b[i] = 200 + i;
    buf.add(a.data(), a.size());
    buf.add(b.data(), b.size());

    REQUIRE(buf.in_buffer == 150);
    CHECK(std::equal(a.begin(), a.end(), buf.buffer_ptr));
    CHECK(std::equal(b.begin(), b.end(), buf.buffer_ptr + 100));
}

TEST_CASE("ViterbiSlidingBuffer del() shifts the remainder down and erases the freed tail", "[common][codings][viterbi]")
{
    viterbi::ViterbiSlidingBuffer buf(1024);
    std::vector<uint8_t> data(600);
    for (int i = 0; i < 600; i++)
        data[i] = i % 100; // never 128
    buf.add(data.data(), data.size());

    buf.del(250);
    REQUIRE(buf.in_buffer == 350);
    CHECK(std::equal(data.begin() + 250, data.end(), buf.buffer_ptr));
    // At most 100 bytes after the remaining data are reset to the erasure value
    for (int i = 350; i < 450; i++)
        REQUIRE(buf.buffer_ptr[i] == 128);
    CHECK(buf.buffer_ptr[450] == data[450]); // left as it was

    buf.del(30);
    REQUIRE(buf.in_buffer == 320);
    CHECK(std::equal(data.begin() + 280, data.end(), buf.buffer_ptr));
    for (int i = 320; i < 350; i++)
        REQUIRE(buf.buffer_ptr[i] == 128);

    buf.del(320);
    CHECK(buf.in_buffer == 0);
}
