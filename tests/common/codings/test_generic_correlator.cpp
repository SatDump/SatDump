#include "common/codings/generic_correlator.h"
#include "common/codings/rotation.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstdint>
#include <vector>

namespace
{
    std::vector<uint8_t> asmBits(uint32_t word = 0x1ACFFC1D)
    {
        std::vector<uint8_t> bits(32);
        for (int i = 0; i < 32; i++)
            bits[i] = (word >> (31 - i)) & 1;
        return bits;
    }

    std::vector<uint8_t> randomBits(int count, uint32_t seed)
    {
        std::vector<uint8_t> bits(count);
        for (auto &b : bits)
        {
            seed = seed * 1664525u + 1013904223u;
            b = seed >> 31;
        }
        return bits;
    }

    std::vector<int8_t> softOf(const std::vector<uint8_t> &bits)
    {
        std::vector<int8_t> soft(bits.size());
        for (size_t i = 0; i < bits.size(); i++)
            soft[i] = bits[i] ? 100 : -100;
        return soft;
    }

    void rot90(std::vector<int8_t> &s)
    {
        for (size_t i = 0; i < s.size(); i += 2)
        {
            int8_t re = s[i], im = s[i + 1];
            s[i] = -im;
            s[i + 1] = re;
        }
    }

    // Soft symbols are scaled by 1/63 before the dot product with +-1 syncword symbols
    const float SOFT_SCALE = 100.0f / 63.0f;
} // namespace

TEST_CASE("CorrelatorGeneric BPSK finds the syncword and its polarity", "[common][codings][correlator]")
{
    const int L = 2048;
    int offset = GENERATE(0, 1, 33, 500, 2000);
    bool inverted = GENERATE(false, true);
    INFO("offset " << offset << ", inverted " << inverted);

    std::vector<uint8_t> sync = asmBits();
    CorrelatorGeneric corr(dsp::BPSK, sync, L);

    std::vector<uint8_t> bits = randomBits(L, offset * 3 + inverted);
    for (int i = 0; i < 32; i++)
        bits[offset + i] = sync[i];
    std::vector<int8_t> soft = softOf(bits);
    if (inverted)
        for (auto &s : soft)
            s = -s;

    phase_t phase;
    bool swap = true;
    float cor = 0;
    CHECK(corr.correlate(soft.data(), phase, swap, cor, L) == offset);
    CHECK(cor == Catch::Approx(32 * SOFT_SCALE).epsilon(1e-4));
    CHECK(phase == (inverted ? PHASE_180 : PHASE_0));
    CHECK(swap == false);

    rotate_soft(soft.data(), L, phase, swap);
    CHECK(soft == softOf(bits));
}

TEST_CASE("CorrelatorGeneric BPSK picks the strongest match with bit errors", "[common][codings][correlator]")
{
    const int L = 1024;
    std::vector<uint8_t> sync = asmBits();
    CorrelatorGeneric corr(dsp::BPSK, sync, L);

    std::vector<uint8_t> bits = randomBits(L, 1);
    for (int i = 0; i < 32; i++)
        bits[100 + i] = sync[i] ^ (i % 8 == 0); // 4 errors
    for (int i = 0; i < 32; i++)
        bits[600 + i] = sync[i] ^ (i == 5); // 1 error, should win
    std::vector<int8_t> soft = softOf(bits);

    phase_t phase;
    bool swap;
    float cor;
    CHECK(corr.correlate(soft.data(), phase, swap, cor, L) == 600);
    CHECK(cor == Catch::Approx(30 * SOFT_SCALE).epsilon(1e-4));
}

TEST_CASE("CorrelatorGeneric QPSK finds and corrects all four rotations", "[common][codings][correlator]")
{
    const int L = 2048;
    int offset = GENERATE(0, 2, 64, 1500);
    int rotation = GENERATE(0, 1, 2, 3);
    INFO("offset " << offset << ", rotation " << rotation * 90);

    std::vector<uint8_t> sync = asmBits();
    CorrelatorGeneric corr(dsp::QPSK, sync, L);

    std::vector<uint8_t> bits = randomBits(L, offset * 5 + rotation);
    for (int i = 0; i < 32; i++)
        bits[offset + i] = sync[i];
    std::vector<int8_t> tx = softOf(bits);
    std::vector<int8_t> rx = tx;
    for (int r = 0; r < rotation; r++)
        rot90(rx);

    phase_t phase;
    bool swap = true;
    float cor;
    CHECK(corr.correlate(rx.data(), phase, swap, cor, L) == offset);
    CHECK(cor == Catch::Approx(32 * SOFT_SCALE).epsilon(1e-3));
    CHECK(phase == (phase_t)rotation);
    CHECK(swap == false);

    rotate_soft(rx.data(), L, phase, swap);
    CHECK(rx == tx);
}

TEST_CASE("CorrelatorGeneric OQPSK reports 90 and 270 degree rotations", "[common][codings][correlator]")
{
    const int L = 1024;
    std::vector<uint8_t> sync = asmBits();
    CorrelatorGeneric corr(dsp::OQPSK, sync, L);

    std::vector<uint8_t> bits = randomBits(L, 9);
    for (int i = 0; i < 32; i++)
        bits[300 + i] = sync[i];
    std::vector<int8_t> tx = softOf(bits);

    for (int rotation : {1, 3})
    {
        INFO("rotation " << rotation * 90);
        std::vector<int8_t> rx = tx;
        for (int r = 0; r < rotation; r++)
            rot90(rx);

        phase_t phase;
        bool swap = true;
        float cor;
        CHECK(corr.correlate(rx.data(), phase, swap, cor, L) == 300);
        CHECK(phase == (rotation == 1 ? PHASE_90 : PHASE_270));
        CHECK(swap == false);

        rotate_soft(rx.data(), L, phase, swap);
        CHECK(rx == tx);
    }
}

TEST_CASE("CorrelatorGeneric OQPSK detects a Q branch delayed by one symbol", "[common][codings][correlator]")
{
    const int L = 1024;
    std::vector<uint8_t> sync = asmBits();
    CorrelatorGeneric corr(dsp::OQPSK, sync, L);

    // Q bits shifted by one symbol, as with the wrong OQPSK delay
    std::vector<uint8_t> bits = randomBits(L, 10);
    std::vector<uint8_t> delayed = sync;
    uint8_t lastQ = 0;
    for (int i = 0; i < 16; i++)
        std::swap(delayed[i * 2 + 1], lastQ);
    for (int i = 0; i < 32; i++)
        bits[400 + i] = delayed[i];

    std::vector<int8_t> rx = softOf(bits);
    phase_t phase;
    bool swap;
    float cor;
    CHECK(corr.correlate(rx.data(), phase, swap, cor, L) == 400);
    CHECK(phase == PHASE_0);
    CHECK(swap == true);
}
