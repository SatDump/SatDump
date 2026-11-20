#include "common/codings/correlator.h"
#include "common/codings/rotation.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstdint>
#include <vector>

namespace
{
    // CCSDS ASM 0x1ACFFC1D after the r=1/2 k=7 convolutional code, as used by the LRPT decoder
    const uint64_t SYNC64 = 0xfca2b63db00d9794;

    // The correlator's internal hard-decision buffer holds 8192 * 20 bytes
    const int HARD_BUF_BYTES = 8192 * 20;

    std::vector<uint8_t> makeBits(int count, uint32_t seed)
    {
        std::vector<uint8_t> bits(count);
        for (auto &b : bits)
        {
            seed = seed * 1664525u + 1013904223u;
            b = seed >> 31;
        }
        return bits;
    }

    void placeWord(std::vector<uint8_t> &bits, int offset, uint64_t word)
    {
        for (int i = 0; i < 64; i++)
            bits[offset + i] = (word >> (63 - i)) & 1;
    }

    std::vector<int8_t> toSoft(const std::vector<uint8_t> &bits)
    {
        std::vector<int8_t> soft(bits.size());
        for (size_t i = 0; i < bits.size(); i++)
            soft[i] = bits[i] ? 100 : -100;
        return soft;
    }

    // Constellation ambiguities on (I, Q) soft pairs
    void rotate90(std::vector<int8_t> &s)
    {
        for (size_t i = 0; i < s.size(); i += 2)
        {
            int8_t re = s[i], im = s[i + 1];
            s[i] = -im;
            s[i + 1] = re;
        }
    }

    void swapIQ(std::vector<int8_t> &s)
    {
        for (size_t i = 0; i < s.size(); i += 2)
            std::swap(s[i], s[i + 1]);
    }

    // Fills the whole internal hard buffer with known data. The correlators scan
    // past the bytes written by the current call (see the tests below), so without
    // this they would read uninitialised memory. A syncword at position 0 makes the
    // priming call return early, before any scanning.
    void prime(Correlator &corr, uint64_t pos0Word, int extraSyncAt = -1)
    {
        std::vector<uint8_t> bits(HARD_BUF_BYTES * 8, 0);
        placeWord(bits, 0, pos0Word);
        if (extraSyncAt >= 0)
            placeWord(bits, extraSyncAt, pos0Word);
        std::vector<int8_t> soft = toSoft(bits);
        phase_t phase;
        bool swap;
        int cor;
        REQUIRE(corr.correlate(soft.data(), phase, swap, cor, soft.size()) == 0);
    }
} // namespace

TEST_CASE("Correlator BPSK finds the syncword at any bit offset", "[common][codings][correlator]")
{
    const int L = 4096;
    int offset = GENERATE(0, 1, 7, 8, 13, 255, 1000, 2047, 3000);
    bool inverted = GENERATE(false, true);
    INFO("offset " << offset << ", inverted " << inverted);

    Correlator corr(BPSK, SYNC64);
    prime(corr, SYNC64);

    std::vector<uint8_t> bits = makeBits(L, offset + 1);
    placeWord(bits, offset, SYNC64);
    std::vector<int8_t> soft = toSoft(bits);
    if (inverted)
        for (auto &s : soft)
            s = -s;

    phase_t phase = PHASE_90;
    bool swap = true;
    int cor = 0;
    int pos = corr.correlate(soft.data(), phase, swap, cor, L);

    CHECK(pos == offset);
    CHECK(cor == 64);
    CHECK(phase == (inverted ? PHASE_180 : PHASE_0));
    CHECK(swap == false);

    // The reported phase corrects the stream
    rotate_soft(soft.data(), L, phase, swap);
    CHECK(soft == toSoft(bits));
}

TEST_CASE("Correlator BPSK tolerates bit errors in the syncword", "[common][codings][correlator]")
{
    const int L = 4096;
    Correlator corr(BPSK, SYNC64);
    prime(corr, SYNC64);

    for (int errors : {1, 4, 10})
    {
        for (int offset : {0, 333})
        {
            INFO("errors " << errors << ", offset " << offset);
            std::vector<uint8_t> bits = makeBits(L, errors * 100 + offset);
            placeWord(bits, offset, SYNC64);
            for (int e = 0; e < errors; e++)
                bits[offset + e * 6] ^= 1;

            std::vector<int8_t> soft = toSoft(bits);
            phase_t phase;
            bool swap;
            int cor;
            int pos = corr.correlate(soft.data(), phase, swap, cor, L);
            CHECK(pos == offset);
            CHECK(cor == 64 - errors);
            CHECK(phase == PHASE_0);
        }
    }
}

TEST_CASE("Correlator BPSK only short-circuits at position 0 above 45 matching bits", "[common][codings][correlator]")
{
    const int L = 2048;
    Correlator corr(BPSK, SYNC64);
    prime(corr, SYNC64);

    // 18 errors at position 0 (46 matches) still returns immediately
    std::vector<uint8_t> bits = makeBits(L, 5);
    placeWord(bits, 0, SYNC64);
    for (int e = 0; e < 18; e++)
        bits[e * 3] ^= 1;
    std::vector<int8_t> soft = toSoft(bits);
    phase_t phase;
    bool swap;
    int cor;
    CHECK(corr.correlate(soft.data(), phase, swap, cor, L) == 0);
    CHECK(cor == 46);

    // A clean syncword later on wins over a weak match at position 0
    for (int e = 18; e < 20; e++)
        bits[e * 3] ^= 1;
    placeWord(bits, 800, SYNC64);
    soft = toSoft(bits);
    CHECK(corr.correlate(soft.data(), phase, swap, cor, L) == 800);
    CHECK(cor == 64);
}

// BUG: correlator.cpp:113 the BPSK scan loops `i < length - 8` over *bytes* although only
// length / 8 bytes were packed, so it reads up to `length` bytes of the internal buffer:
// stale data from earlier calls (or uninitialised memory) is searched too, and positions
// beyond the input can be returned. The QPSK branch correctly uses (length / 8) - 8.
TEST_CASE("Correlator BPSK never reports a position outside the input", "[common][codings][correlator][!shouldfail]")
{
    const int L = 4096;
    Correlator corr(BPSK, SYNC64);
    prime(corr, SYNC64, 16000); // leaves a syncword at bit 16000 of the internal buffer

    std::vector<int8_t> soft = toSoft(makeBits(L, 77)); // no syncword in this input
    phase_t phase;
    bool swap;
    int cor;
    int pos = corr.correlate(soft.data(), phase, swap, cor, L);
    CHECK(pos <= L - 64);
    CHECK(cor < 64);
}

TEST_CASE("Correlator QPSK finds the syncword at even offsets for every rotation", "[common][codings][correlator]")
{
    const int L = 4096;
    int offset = GENERATE(0, 2, 10, 64, 1000, 2950);
    int rotation = GENERATE(0, 1, 2, 3);
    INFO("offset " << offset << ", rotation " << rotation * 90);

    Correlator corr(QPSK, SYNC64);
    prime(corr, SYNC64);

    // The complement of the constructor syncword is the unrotated pattern
    std::vector<uint8_t> bits = makeBits(L, offset * 4 + rotation);
    placeWord(bits, offset, ~SYNC64);
    std::vector<int8_t> tx = toSoft(bits);

    std::vector<int8_t> rx = tx;
    for (int r = 0; r < rotation; r++)
        rotate90(rx);

    phase_t phase;
    bool swap;
    int cor = 0;
    int pos = corr.correlate(rx.data(), phase, swap, cor, L);
    CHECK(pos == offset);
    CHECK(cor == 64);
    CHECK(swap == false);
    if (rotation == 0)
        CHECK(phase == PHASE_0);

    rotate_soft(rx.data(), L, phase, swap);
    CHECK(rx == tx);
}

TEST_CASE("Correlator QPSK tolerates bit errors", "[common][codings][correlator]")
{
    const int L = 4096;
    Correlator corr(QPSK, SYNC64);
    prime(corr, SYNC64);

    for (int errors : {1, 5, 12})
    {
        std::vector<uint8_t> bits = makeBits(L, errors);
        placeWord(bits, 500, ~SYNC64);
        for (int e = 0; e < errors; e++)
            bits[500 + e * 5] ^= 1;
        std::vector<int8_t> rx = toSoft(bits);
        rotate90(rx);

        phase_t phase;
        bool swap;
        int cor;
        INFO("errors " << errors);
        CHECK(corr.correlate(rx.data(), phase, swap, cor, L) == 500);
        CHECK(cor == 64 - errors);
    }
}

// BUG: for IQ-swapped input the (phase, swap) pair returned by Correlator::correlate does
// not always undo the ambiguity through rotate_soft(): the swapped syncwords built in
// correlator.cpp:54-62 are swap(rotate(~swap(sync))) while rotate_soft() applies the swap
// before the rotation, so for two of the four swapped orientations the corrected stream
// ends up rotated by 180 degrees compared to the other six cases.
TEST_CASE("Correlator QPSK corrections for IQ-swapped input restore the stream", "[common][codings][correlator][!shouldfail]")
{
    const int L = 2048;
    Correlator corr(QPSK, SYNC64);
    prime(corr, SYNC64);

    std::vector<uint8_t> bits = makeBits(L, 4321);
    placeWord(bits, 200, ~SYNC64);
    std::vector<int8_t> tx = toSoft(bits);

    int failures = 0;
    for (int rotation = 0; rotation < 4; rotation++)
    {
        std::vector<int8_t> rx = tx;
        for (int r = 0; r < rotation; r++)
            rotate90(rx);
        swapIQ(rx);

        phase_t phase;
        bool swap;
        int cor;
        int pos = corr.correlate(rx.data(), phase, swap, cor, L);
        REQUIRE(pos == 200);
        REQUIRE(cor == 64);
        CHECK(swap == true);

        rotate_soft(rx.data(), L, phase, swap);
        failures += rx != tx;
    }
    CHECK(failures == 0);
}
