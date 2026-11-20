#include "common/codings/viterbi/cc_encoder.h"
#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <vector>

namespace
{
    std::vector<uint8_t> randomBits(int count, uint32_t seed)
    {
        std::vector<uint8_t> bits(count);
        uint32_t state = seed;
        for (auto &b : bits)
        {
            state = state * 1664525u + 1013904223u;
            b = state >> 31;
        }
        return bits;
    }

    // CCSDS 131.0-B k=7 r=1/2 code written from the standard: G1 = 171 (octal), G2 = 133 (octal),
    // leftmost tap = current input, then delays 1..6. No output inversion.
    std::vector<uint8_t> referenceCcsdsEncode(const std::vector<uint8_t> &bits)
    {
        std::vector<uint8_t> out;
        uint8_t d[7] = {0}; // d[0] = current, d[i] = input delayed by i
        for (uint8_t b : bits)
        {
            for (int i = 6; i > 0; i--)
                d[i] = d[i - 1];
            d[0] = b;
            out.push_back(d[0] ^ d[1] ^ d[2] ^ d[3] ^ d[6]); // 1111001
            out.push_back(d[0] ^ d[2] ^ d[3] ^ d[5] ^ d[6]); // 1011011
        }
        return out;
    }
}

TEST_CASE("CCEncoder matches the CCSDS k=7 r=1/2 reference encoder", "[common][codings][viterbi]")
{
    std::vector<uint8_t> bits = randomBits(4096, 3);
    viterbi::CCEncoder enc(bits.size(), 7, 2, {79, 109});
    std::vector<uint8_t> out(bits.size() * 2);
    enc.work(bits.data(), out.data());
    CHECK(out == referenceCcsdsEncode(bits));
}

TEST_CASE("CCEncoder impulse response gives the generator polynomials", "[common][codings][viterbi]")
{
    std::vector<uint8_t> bits(8, 0);
    bits[0] = 1;
    viterbi::CCEncoder enc(bits.size(), 7, 2, {79, 109});
    std::vector<uint8_t> out(16);
    enc.work(bits.data(), out.data());
    // 79 = 0b1001111 and 109 = 0b1101101, LSB first (LSB = current input)
    std::vector<uint8_t> expected = {1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0};
    CHECK(out == expected);
}

TEST_CASE("CCEncoder only uses the LSB of each input byte", "[common][codings][viterbi]")
{
    std::vector<uint8_t> bits = randomBits(256, 4);
    std::vector<uint8_t> dirty = bits;
    for (auto &b : dirty)
        b |= 0xFE;
    viterbi::CCEncoder a(bits.size(), 7, 2, {79, 109});
    viterbi::CCEncoder b(bits.size(), 7, 2, {79, 109});
    std::vector<uint8_t> outA(512), outB(512);
    a.work(bits.data(), outA.data());
    b.work(dirty.data(), outB.data());
    CHECK(outA == outB);
}

TEST_CASE("CCEncoder negative polynomials invert that output", "[common][codings][viterbi]")
{
    std::vector<uint8_t> bits = randomBits(512, 5);
    viterbi::CCEncoder normal(bits.size(), 7, 2, {79, 109});
    viterbi::CCEncoder inverted(bits.size(), 7, 2, {79, -109});
    std::vector<uint8_t> a(1024), b(1024);
    normal.work(bits.data(), a.data());
    inverted.work(bits.data(), b.data());
    for (size_t i = 0; i < a.size(); i++)
    {
        INFO("symbol " << i);
        REQUIRE(b[i] == (i % 2 ? (a[i] ^ 1) : a[i]));
    }
}

TEST_CASE("CCEncoder keeps the shift register state across work() calls", "[common][codings][viterbi]")
{
    std::vector<uint8_t> bits = randomBits(1000, 6);
    std::vector<uint8_t> reference = referenceCcsdsEncode(bits);

    SECTION("fixed frame size")
    {
        viterbi::CCEncoder enc(250, 7, 2, {79, 109});
        std::vector<uint8_t> out(2000);
        for (int f = 0; f < 4; f++)
            enc.work(&bits[f * 250], &out[f * 500]);
        CHECK(out == reference);
    }

    SECTION("explicit sizes")
    {
        viterbi::CCEncoder enc(1000, 7, 2, {79, 109});
        std::vector<uint8_t> out(2000);
        int done = 0;
        for (int size : {1, 7, 100, 333, 559})
        {
            enc.work(&bits[done], &out[done * 2], size);
            done += size;
        }
        REQUIRE(done == 1000);
        CHECK(out == reference);
    }
}

TEST_CASE("CCEncoder start state is used as the initial register content", "[common][codings][viterbi]")
{
    // Start state 0b111111 is equivalent to having encoded six ones first
    std::vector<uint8_t> bits = randomBits(100, 7);
    std::vector<uint8_t> prefixed(6, 1);
    prefixed.insert(prefixed.end(), bits.begin(), bits.end());
    std::vector<uint8_t> reference = referenceCcsdsEncode(prefixed);

    viterbi::CCEncoder enc(bits.size(), 7, 2, {79, 109}, 0x3F);
    std::vector<uint8_t> out(200);
    enc.work(bits.data(), out.data());
    CHECK(std::equal(out.begin(), out.end(), reference.begin() + 12));
}

TEST_CASE("CCEncoder set_frame_size clamps to the constructed maximum", "[common][codings][viterbi]")
{
    viterbi::CCEncoder enc(100, 7, 2, {79, 109});
    CHECK(enc.set_frame_size(50));
    CHECK(enc.set_frame_size(100));
    CHECK_FALSE(enc.set_frame_size(101));

    // After clamping the frame size is the maximum: 100 bits -> 200 symbols written
    std::vector<uint8_t> bits = randomBits(100, 8);
    std::vector<uint8_t> out(210, 0xAA);
    enc.work(bits.data(), out.data());
    CHECK(out[199] != 0xAA);
    CHECK(out[200] == 0xAA);

    enc.set_frame_size(10);
    std::vector<uint8_t> out2(30, 0xAA);
    enc.work(bits.data(), out2.data());
    CHECK(out2[19] != 0xAA);
    CHECK(out2[20] == 0xAA);
}

TEST_CASE("CCEncoder rate() returns the inverse rate", "[common][codings][viterbi]")
{
    // Unlike CCDecoder::rate(), which returns 1/r
    viterbi::CCEncoder enc(10, 7, 2, {79, 109});
    CHECK(enc.rate() == 2.0);
}

TEST_CASE("CCEncoder rejects invalid parameters", "[common][codings][viterbi]")
{
    CHECK_THROWS_AS(viterbi::CCEncoder(10, 7, 2, {79}), std::runtime_error);         // poly count != rate
    CHECK_THROWS_AS(viterbi::CCEncoder(10, 7, 1, {79}), std::runtime_error);         // rate < 2
    CHECK_THROWS_AS(viterbi::CCEncoder(10, 1, 2, {79, 109}), std::runtime_error);    // k too small
    CHECK_THROWS_AS(viterbi::CCEncoder(10, 32, 2, {79, 109}), std::runtime_error);   // k too large
    CHECK_THROWS_AS(viterbi::CCEncoder(10, 7, 2, {79, 109}, 64), std::runtime_error); // start state >= 2^(k-1)
    CHECK_THROWS_AS(viterbi::CCEncoder(0, 7, 2, {79, 109}), std::runtime_error);     // empty frame
    CHECK_NOTHROW(viterbi::CCEncoder(10, 7, 2, {79, 109}, 63));
    CHECK_NOTHROW(viterbi::CCEncoder(10, 3, 3, {7, 5, 3}));
}
