#include "common/codings/correlator32.h"
#include "common/codings/rotation.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstdint>
#include <vector>

namespace
{
    const uint32_t ASM32 = 0x1ACFFC1D;
    const int HARD_BUF_BYTES_32 = 8192 * 20;

    std::vector<uint8_t> makeBits32(int count, uint32_t seed)
    {
        std::vector<uint8_t> bits(count);
        for (auto &b : bits)
        {
            seed = seed * 1664525u + 1013904223u;
            b = seed >> 31;
        }
        return bits;
    }

    void placeWord32(std::vector<uint8_t> &bits, int offset, uint32_t word)
    {
        for (int i = 0; i < 32; i++)
            bits[offset + i] = (word >> (31 - i)) & 1;
    }

    std::vector<int8_t> toSoft32(const std::vector<uint8_t> &bits)
    {
        std::vector<int8_t> soft(bits.size());
        for (size_t i = 0; i < bits.size(); i++)
            soft[i] = bits[i] ? 100 : -100;
        return soft;
    }

    void rotate90Pairs(std::vector<int8_t> &s)
    {
        for (size_t i = 0; i < s.size(); i += 2)
        {
            int8_t re = s[i], im = s[i + 1];
            s[i] = -im;
            s[i + 1] = re;
        }
    }

    // Fills the internal hard buffer with known data so later scans never read
    // uninitialised memory; a syncword at position 0 makes this call return early.
    void prime32(Correlator32 &corr, uint32_t pos0Word, int extraSyncAt = -1)
    {
        std::vector<uint8_t> bits(HARD_BUF_BYTES_32 * 8, 0);
        placeWord32(bits, 0, pos0Word);
        if (extraSyncAt >= 0)
            placeWord32(bits, extraSyncAt, pos0Word);
        std::vector<int8_t> soft = toSoft32(bits);
        phase_t phase;
        bool swap;
        int cor;
        REQUIRE(corr.correlate(soft.data(), phase, swap, cor, soft.size()) == 0);
    }
} // namespace

TEST_CASE("Correlator32 BPSK finds the ASM at any bit offset, normal or inverted", "[common][codings][correlator]")
{
    const int L = 2048;
    int offset = GENERATE(0, 1, 5, 8, 31, 100, 1023, 1900);
    bool inverted = GENERATE(false, true);
    INFO("offset " << offset << ", inverted " << inverted);

    Correlator32 corr(BPSK, ASM32);
    prime32(corr, ASM32);

    std::vector<uint8_t> bits = makeBits32(L, offset * 2 + inverted);
    placeWord32(bits, offset, ASM32);
    std::vector<int8_t> soft = toSoft32(bits);
    if (inverted)
        for (auto &s : soft)
            s = -s;

    phase_t phase;
    bool swap = true;
    int cor = 0;
    CHECK(corr.correlate(soft.data(), phase, swap, cor, L) == offset);
    CHECK(cor == 32);
    CHECK(phase == (inverted ? PHASE_180 : PHASE_0));
    CHECK(swap == false);

    rotate_soft(soft.data(), L, phase, swap);
    CHECK(soft == toSoft32(bits));
}

TEST_CASE("Correlator32 BPSK tolerates bit errors", "[common][codings][correlator]")
{
    const int L = 2048;
    Correlator32 corr(BPSK, ASM32);
    prime32(corr, ASM32);

    for (int errors : {1, 3, 4})
    {
        for (int offset : {0, 411})
        {
            INFO("errors " << errors << ", offset " << offset);
            std::vector<uint8_t> bits = makeBits32(L, errors * 10 + offset);
            placeWord32(bits, offset, ASM32);
            for (int e = 0; e < errors; e++)
                bits[offset + e * 7] ^= 1;
            std::vector<int8_t> soft = toSoft32(bits);

            phase_t phase;
            bool swap;
            int cor;
            CHECK(corr.correlate(soft.data(), phase, swap, cor, L) == offset);
            CHECK(cor == 32 - errors);
        }
    }
}

TEST_CASE("Correlator32 treats a zero soft symbol as a 1", "[common][codings][correlator]")
{
    const int L = 1024;
    Correlator32 corr(BPSK, ASM32);
    prime32(corr, ASM32);

    std::vector<uint8_t> bits = makeBits32(L, 3);
    placeWord32(bits, 200, ASM32);
    std::vector<int8_t> soft = toSoft32(bits);
    for (int i = 200; i < 232; i++)
        if (bits[i])
            soft[i] = 0;

    phase_t phase;
    bool swap;
    int cor;
    CHECK(corr.correlate(soft.data(), phase, swap, cor, L) == 200);
    CHECK(cor == 32);
}

TEST_CASE("Correlator32 QPSK finds and corrects all four rotations", "[common][codings][correlator]")
{
    const int L = 2048;
    int offset = GENERATE(0, 2, 40, 1000);
    int rotation = GENERATE(0, 1, 2, 3);
    INFO("offset " << offset << ", rotation " << rotation * 90);

    Correlator32 corr(QPSK, ASM32);
    prime32(corr, ASM32);

    std::vector<uint8_t> bits = makeBits32(L, offset + 17 * rotation);
    placeWord32(bits, offset, ~ASM32); // complement of the constructor word is the unrotated pattern
    std::vector<int8_t> tx = toSoft32(bits);
    std::vector<int8_t> rx = tx;
    for (int r = 0; r < rotation; r++)
        rotate90Pairs(rx);

    phase_t phase;
    bool swap;
    int cor;
    CHECK(corr.correlate(rx.data(), phase, swap, cor, L) == offset);
    CHECK(cor == 32);
    CHECK(swap == false);

    rotate_soft(rx.data(), L, phase, swap);
    CHECK(rx == tx);
}

// BUG: correlator32.cpp:152 the QPSK scan loops `i < length - 8` over bytes although only
// length / 8 bytes were packed, so stale buffer contents from earlier calls (or
// uninitialised memory) are searched and a position beyond the input can be returned.
TEST_CASE("Correlator32 QPSK never reports a position outside the input", "[common][codings][correlator][!shouldfail]")
{
    const int L = 2048;
    Correlator32 corr(QPSK, ASM32);
    prime32(corr, ASM32, 12000);

    std::vector<int8_t> soft = toSoft32(makeBits32(L, 55));
    phase_t phase;
    bool swap;
    int cor;
    int pos = corr.correlate(soft.data(), phase, swap, cor, L);
    CHECK(pos <= L - 32);
    CHECK(cor < 32);
}

// BUG: same as Correlator (correlator32.cpp:54-62): for two of the four IQ-swapped
// orientations, rotate_soft(phase, swap) leaves the stream rotated by 180 degrees.
TEST_CASE("Correlator32 QPSK corrections for IQ-swapped input restore the stream", "[common][codings][correlator][!shouldfail]")
{
    const int L = 2048;
    Correlator32 corr(QPSK, ASM32);
    prime32(corr, ASM32);

    std::vector<uint8_t> bits = makeBits32(L, 999);
    placeWord32(bits, 300, ~ASM32);
    std::vector<int8_t> tx = toSoft32(bits);

    int failures = 0;
    for (int rotation = 0; rotation < 4; rotation++)
    {
        std::vector<int8_t> rx = tx;
        for (int r = 0; r < rotation; r++)
            rotate90Pairs(rx);
        for (size_t i = 0; i < rx.size(); i += 2)
            std::swap(rx[i], rx[i + 1]);

        phase_t phase;
        bool swap;
        int cor;
        REQUIRE(corr.correlate(rx.data(), phase, swap, cor, L) == 300);
        CHECK(swap == true);
        rotate_soft(rx.data(), L, phase, swap);
        failures += rx != tx;
    }
    CHECK(failures == 0);
}
