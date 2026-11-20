#include "common/codings/viterbi/cc_decoder.h"
#include "common/codings/viterbi/cc_encoder.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <stdexcept>
#include <vector>

namespace
{
    struct Lcg
    {
        uint32_t state;
        explicit Lcg(uint32_t seed) : state(seed) {}
        uint32_t next()
        {
            state = state * 1664525u + 1013904223u;
            return state;
        }
        int below(int n) { return (next() >> 8) % n; }
    };

    std::vector<uint8_t> randomBits(int count, uint32_t seed)
    {
        Lcg rng(seed);
        std::vector<uint8_t> bits(count);
        for (auto &b : bits)
            b = rng.next() >> 31;
        return bits;
    }

    std::vector<uint8_t> encode(const std::vector<uint8_t> &bits)
    {
        viterbi::CCEncoder enc(bits.size(), 7, 2, {79, 109});
        std::vector<uint8_t> out(bits.size() * 2);
        enc.work((uint8_t *)bits.data(), out.data());
        return out;
    }

    // Decoder soft symbol convention: 0 = strong 0, 255 = strong 1, 128 = erasure.
    // The decoder reads (frame + 6) symbol pairs, so pad with erasures.
    std::vector<uint8_t> toSoft(const std::vector<uint8_t> &syms, int pad = 64)
    {
        std::vector<uint8_t> soft(syms.size() + pad, 128);
        for (size_t i = 0; i < syms.size(); i++)
            soft[i] = syms[i] ? 255 : 0;
        return soft;
    }

    int countDiff(const uint8_t *a, const uint8_t *b, int n)
    {
        int d = 0;
        for (int i = 0; i < n; i++)
            d += a[i] != b[i];
        return d;
    }
}

TEST_CASE("CCDecoder decodes a clean single frame", "[common][codings][viterbi]")
{
    int frame = GENERATE(64, 1024, 8192);
    std::vector<uint8_t> bits = randomBits(frame, frame);
    std::vector<uint8_t> soft = toSoft(encode(bits));

    viterbi::CCDecoder dec(frame, 7, 2, {79, 109});
    std::vector<uint8_t> out(frame, 0xFF);
    dec.work(soft.data(), out.data());
    CHECK(out == bits);
}

TEST_CASE("CCDecoder chains consecutive frames of a continuous stream", "[common][codings][viterbi]")
{
    const int frame = 1024, frames = 6;
    std::vector<uint8_t> bits = randomBits(frame * frames, 11);
    std::vector<uint8_t> syms = encode(bits);

    SECTION("next frame's symbols as look-ahead")
    {
        std::vector<uint8_t> soft = toSoft(syms);
        viterbi::CCDecoder dec(frame, 7, 2, {79, 109});
        for (int f = 0; f < frames; f++)
        {
            INFO("frame " << f);
            std::vector<uint8_t> out(frame);
            dec.work(&soft[f * frame * 2], out.data());
            CHECK(countDiff(out.data(), &bits[f * frame], frame) == 0);
        }
    }

    SECTION("erasures as look-ahead")
    {
        viterbi::CCDecoder dec(frame, 7, 2, {79, 109});
        for (int f = 0; f < frames; f++)
        {
            INFO("frame " << f);
            std::vector<uint8_t> chunk(syms.begin() + f * frame * 2, syms.begin() + (f + 1) * frame * 2);
            std::vector<uint8_t> soft = toSoft(chunk);
            std::vector<uint8_t> out(frame);
            dec.work(soft.data(), out.data());
            CHECK(countDiff(out.data(), &bits[f * frame], frame) == 0);
        }
    }
}

TEST_CASE("CCDecoder corrects sparse hard symbol errors", "[common][codings][viterbi]")
{
    const int frame = 4096;
    std::vector<uint8_t> bits = randomBits(frame, 12);
    std::vector<uint8_t> soft = toSoft(encode(bits));

    // One flipped symbol every ~40 symbols, well within the free distance (10) per constraint span
    Lcg rng(99);
    int flips = 0;
    for (int i = 5; i < frame * 2 - 40; i += 30 + rng.below(20))
    {
        soft[i] = 255 - soft[i];
        flips++;
    }
    REQUIRE(flips > 150);

    viterbi::CCDecoder dec(frame, 7, 2, {79, 109});
    std::vector<uint8_t> out(frame);
    dec.work(soft.data(), out.data());
    CHECK(out == bits);
}

TEST_CASE("CCDecoder uses soft confidence to correct dense weak errors", "[common][codings][viterbi]")
{
    const int frame = 4096;
    std::vector<uint8_t> bits = randomBits(frame, 13);
    std::vector<uint8_t> syms = encode(bits);
    std::vector<uint8_t> soft(syms.size() + 64, 128);

    // 15% of the symbols are on the wrong side but with very low confidence
    Lcg rng(5);
    int wrong = 0;
    for (size_t i = 0; i < syms.size(); i++)
    {
        bool flip = rng.below(100) < 15;
        wrong += flip;
        bool v = syms[i] ^ flip;
        soft[i] = flip ? (v ? 133 : 122) : (v ? 230 : 25);
    }
    REQUIRE(wrong > 1000);

    viterbi::CCDecoder dec(frame, 7, 2, {79, 109});
    std::vector<uint8_t> out(frame);
    dec.work(soft.data(), out.data());
    CHECK(out == bits);
}

TEST_CASE("CCDecoder treats 128 as an erasure (depunctured rate 2/3 and 3/4)", "[common][codings][viterbi]")
{
    const int frame = 4096;
    std::vector<uint8_t> bits = randomBits(frame, 14);
    std::vector<uint8_t> soft = toSoft(encode(bits));

    SECTION("every fourth symbol erased (X1 Y1 Y2)")
    {
        for (int i = 2; i < frame * 2; i += 4)
            soft[i] = 128;
    }
    SECTION("DVB 3/4 pattern X1 Y1 Y2 X3")
    {
        for (int b = 0; b < frame; b++)
        {
            if (b % 3 == 1)
                soft[b * 2] = 128;
            if (b % 3 == 2)
                soft[b * 2 + 1] = 128;
        }
    }

    viterbi::CCDecoder dec(frame, 7, 2, {79, 109});
    std::vector<uint8_t> out(frame);
    dec.work(soft.data(), out.data());
    CHECK(out == bits);
}

TEST_CASE("CCDecoder decodes noisy symbols at least as well as hard decisions", "[common][codings][viterbi]")
{
    const int frame = 8192;
    std::vector<uint8_t> bits = randomBits(frame, 15);
    std::vector<uint8_t> syms = encode(bits);
    std::vector<uint8_t> soft(syms.size() + 64, 128);

    // Random hard flips at a 3% channel error rate: the decoded error rate must be far lower
    Lcg rng(21);
    int channelErrors = 0;
    for (size_t i = 0; i < syms.size(); i++)
    {
        bool flip = rng.below(1000) < 30;
        channelErrors += flip;
        soft[i] = (syms[i] ^ flip) ? 255 : 0;
    }

    viterbi::CCDecoder dec(frame, 7, 2, {79, 109});
    std::vector<uint8_t> out(frame);
    dec.work(soft.data(), out.data());
    int decodedErrors = countDiff(out.data(), bits.data(), frame);
    INFO("channel errors " << channelErrors << ", decoded errors " << decodedErrors);
    CHECK(decodedErrors * 20 < channelErrors);
}

TEST_CASE("CCDecoder set_frame_size decodes shorter frames and clamps to the maximum", "[common][codings][viterbi]")
{
    viterbi::CCDecoder dec(2048, 7, 2, {79, 109});
    CHECK_FALSE(dec.set_frame_size(4096));
    CHECK(dec.set_frame_size(512));

    std::vector<uint8_t> bits = randomBits(512, 16);
    std::vector<uint8_t> soft = toSoft(encode(bits));
    std::vector<uint8_t> out(2048, 0xEE);
    dec.work(soft.data(), out.data());
    CHECK(std::equal(bits.begin(), bits.end(), out.begin()));
    CHECK(out[512] == 0xEE);
}

TEST_CASE("CCDecoder rate() and unsupported parameters", "[common][codings][viterbi]")
{
    viterbi::CCDecoder dec(100, 7, 2, {79, 109});
    CHECK(dec.rate() == 0.5);

    // Only k=7 r=1/2 has a kernel
    CHECK_THROWS_AS(viterbi::CCDecoder(100, 5, 2, {19, 27}), std::runtime_error);
    CHECK_THROWS_AS(viterbi::CCDecoder(100, 7, 3, {79, 109, 91}), std::runtime_error);
}

TEST_CASE("CCDecoder inverted polynomial matches inverted encoder", "[common][codings][viterbi]")
{
    const int frame = 1024;
    std::vector<uint8_t> bits = randomBits(frame, 17);
    viterbi::CCEncoder enc(frame, 7, 2, {79, -109});
    std::vector<uint8_t> syms(frame * 2);
    enc.work(bits.data(), syms.data());
    std::vector<uint8_t> soft = toSoft(syms);

    viterbi::CCDecoder dec(frame, 7, 2, {79, -109});
    std::vector<uint8_t> out(frame);
    dec.work(soft.data(), out.data());
    CHECK(out == bits);
}
