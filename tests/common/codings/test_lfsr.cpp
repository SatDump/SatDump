#include "common/codings/lfsr.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace
{
    std::vector<uint8_t> runLfsr(common::lfsr &l, int count)
    {
        std::vector<uint8_t> bits(count);
        for (auto &b : bits)
            b = l.next_bit();
        return bits;
    }

    int period(const std::vector<uint8_t> &bits, int maxPeriod)
    {
        for (int p = 1; p <= maxPeriod; p++)
        {
            bool ok = true;
            for (size_t i = 0; i + p < bits.size() && ok; i++)
                ok = bits[i] == bits[i + p];
            if (ok)
                return p;
        }
        return -1;
    }
} // namespace

TEST_CASE("lfsr generates maximal length sequences for the documented masks", "[common][codings][lfsr]")
{
    struct Case
    {
        uint64_t mask;
        uint8_t len;
        int expectedPeriod;
    };
    // Masks from the class documentation: x^4+x^3+1, x^5+x^3+1, x^6+x^5+1
    for (Case c : {Case{0x19, 3, 15}, Case{0x29, 4, 31}, Case{0x61, 5, 63}})
    {
        INFO("mask " << c.mask);
        common::lfsr l(c.mask, 1, c.len);
        std::vector<uint8_t> bits = runLfsr(l, c.expectedPeriod * 4);
        CHECK(period(bits, c.expectedPeriod * 2) == c.expectedPeriod);

        // A maximal length sequence has 2^(n-1) ones per period
        int ones = 0;
        for (int i = 0; i < c.expectedPeriod; i++)
            ones += bits[i];
        CHECK(ones == (c.expectedPeriod + 1) / 2);
    }
}

TEST_CASE("lfsr configured as the CCSDS randomizer reproduces its sequence", "[common][codings][lfsr]")
{
    // h(x) = x^8 + x^7 + x^5 + x^3 + 1: a[n+8] = a[n] ^ a[n+3] ^ a[n+5] ^ a[n+7],
    // so taps on register bits 0, 3, 5, 7 and the new bit enters at bit 7
    common::lfsr l(0b10101001, 0xFF, 7);
    std::vector<uint8_t> bits = runLfsr(l, 255 * 2);

    uint8_t expectedBytes[] = {0xFF, 0x48, 0x0E, 0xC0, 0x9A, 0x0D, 0x70, 0xBC};
    for (int i = 0; i < 64; i++)
        REQUIRE(bits[i] == ((expectedBytes[i / 8] >> (7 - i % 8)) & 1));
    CHECK(period(bits, 300) == 255);
}

TEST_CASE("lfsr reset and pre_shift reposition the sequence", "[common][codings][lfsr]")
{
    common::lfsr a(0x29, 0x1F, 4);
    std::vector<uint8_t> first = runLfsr(a, 40);

    a.reset();
    CHECK(runLfsr(a, 40) == first);

    common::lfsr b(0x29, 0x1F, 4);
    b.pre_shift(7);
    std::vector<uint8_t> shifted = runLfsr(b, 33);
    CHECK(shifted == std::vector<uint8_t>(first.begin() + 7, first.end()));

    CHECK(a.mask() == 0x29);
}

TEST_CASE("lfsr first output bits are the seed, LSB first", "[common][codings][lfsr]")
{
    common::lfsr l(0x61, 0b101101, 5);
    std::vector<uint8_t> bits = runLfsr(l, 6);
    CHECK(bits == std::vector<uint8_t>{1, 0, 1, 1, 0, 1});
}

TEST_CASE("lfsr rejects register lengths above 63", "[common][codings][lfsr]")
{
    CHECK_THROWS_AS(common::lfsr(1, 1, 64), std::invalid_argument);
    CHECK_NOTHROW(common::lfsr(1, 1, 63));
}

TEST_CASE("lfsr descrambler inverts the multiplicative scrambler", "[common][codings][lfsr]")
{
    const uint64_t mask = 0x8A; // arbitrary tap set
    const uint8_t len = 16;
    const int delay = len + 1; // the scrambler output is taken from the register LSB

    std::vector<uint8_t> input(2000);
    uint32_t state = 12345;
    for (auto &b : input)
    {
        state = state * 1664525u + 1013904223u;
        b = state >> 31;
    }

    common::lfsr scrambler(mask, 0, len);
    std::vector<uint8_t> scrambled(input.size());
    for (size_t i = 0; i < input.size(); i++)
        scrambled[i] = scrambler.next_bit_scramble(input[i]);

    SECTION("same seed")
    {
        common::lfsr descrambler(mask, 0, len);
        std::vector<uint8_t> out(input.size());
        for (size_t i = 0; i < input.size(); i++)
            out[i] = descrambler.next_bit_descramble(scrambled[i]);
        for (size_t i = 2 * delay; i < input.size(); i++)
            REQUIRE(out[i] == input[i - delay]);
    }

    SECTION("self-synchronising even with a wrong seed")
    {
        common::lfsr descrambler(mask, 0xBEEF, len);
        std::vector<uint8_t> out(input.size());
        for (size_t i = 0; i < input.size(); i++)
            out[i] = descrambler.next_bit_descramble(scrambled[i]);
        for (size_t i = 2 * delay + 1; i < input.size(); i++)
            REQUIRE(out[i] == input[i - delay]);
    }

    SECTION("scrambling changes the data")
    {
        int differences = 0;
        for (size_t i = delay; i < input.size(); i++)
            differences += scrambled[i] != input[i - delay];
        CHECK(differences > 500);
    }
}
