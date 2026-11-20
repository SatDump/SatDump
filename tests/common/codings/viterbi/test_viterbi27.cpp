#include "common/codings/viterbi/viterbi27.h"
#include "helpers/random.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
    using test_helpers::randomBits;

    std::vector<uint8_t> encode(const std::vector<uint8_t> &bits)
    {
        viterbi::CCEncoder enc(bits.size(), 7, 2, {79, 109});
        std::vector<uint8_t> out(bits.size() * 2);
        enc.work((uint8_t *)bits.data(), out.data());
        return out;
    }

    // Signed soft symbols: positive = 1
    std::vector<int8_t> toSigned(const std::vector<uint8_t> &syms, int begin, int count)
    {
        std::vector<int8_t> soft(count);
        for (int i = 0; i < count; i++)
            soft[i] = syms[begin + i] ? 100 : -100;
        return soft;
    }

    std::vector<uint8_t> packMsbFirst(const uint8_t *bits, int count)
    {
        std::vector<uint8_t> bytes(count / 8, 0);
        for (int i = 0; i < count; i++)
            bytes[i / 8] |= bits[i] << (7 - (i % 8));
        return bytes;
    }
} // namespace

TEST_CASE("Viterbi27 decodes clean frames into MSB-first packed bytes", "[common][codings][viterbi]")
{
    const int frame = 2048, frames = 4;
    std::vector<uint8_t> bits = randomBits(frame * frames, 1);
    std::vector<uint8_t> syms = encode(bits);

    viterbi::Viterbi27 vit(frame, viterbi::CCSDS_R2_K7_POLYS);
    for (int f = 0; f < frames; f++)
    {
        INFO("frame " << f);
        std::vector<int8_t> soft = toSigned(syms, f * frame * 2, frame * 2);
        std::vector<uint8_t> out(frame / 8);
        vit.work(soft.data(), out.data());
        CHECK(out == packMsbFirst(&bits[f * frame], frame));
    }
}

TEST_CASE("Viterbi27 hard mode takes unsigned 0/255 symbols", "[common][codings][viterbi]")
{
    const int frame = 1024;
    std::vector<uint8_t> bits = randomBits(frame, 2);
    std::vector<uint8_t> syms = encode(bits);
    for (auto &s : syms)
        s = s ? 255 : 0;

    viterbi::Viterbi27 vit(frame, viterbi::CCSDS_R2_K7_POLYS);
    std::vector<uint8_t> out(frame / 8);
    vit.work((int8_t *)syms.data(), out.data(), true);
    CHECK(out == packMsbFirst(bits.data(), frame));
    CHECK(vit.ber() == 0.0f);
}

TEST_CASE("Viterbi27 ber() is 4x the channel symbol error rate over the first 1024 symbols", "[common][codings][viterbi]")
{
    const int frame = 2048;
    std::vector<uint8_t> bits = randomBits(frame, 3);
    std::vector<uint8_t> syms = encode(bits);
    std::vector<int8_t> soft = toSigned(syms, 0, frame * 2);

    // 16 isolated flips inside the BER window, plus some outside it which must not count
    int flipsInWindow = 0;
    for (int i = 10; i < 1024; i += 64)
    {
        soft[i] = -soft[i];
        flipsInWindow++;
    }
    for (int i = 1500; i < 3500; i += 100)
        soft[i] = -soft[i];

    viterbi::Viterbi27 vit(frame, viterbi::CCSDS_R2_K7_POLYS);
    std::vector<uint8_t> out(frame / 8);
    vit.work(soft.data(), out.data());

    CHECK(out == packMsbFirst(bits.data(), frame));
    CHECK(vit.ber() == Catch::Approx(4.0 * flipsInWindow / 1024.0));
}

TEST_CASE("Viterbi27 ber() on a clean first frame is zero, on random data it is high", "[common][codings][viterbi]")
{
    const int frame = 2048;
    std::vector<uint8_t> out(frame / 8);

    std::vector<uint8_t> bits = randomBits(frame, 4);
    std::vector<int8_t> clean = toSigned(encode(bits), 0, frame * 2);
    viterbi::Viterbi27 a(frame, viterbi::CCSDS_R2_K7_POLYS);
    a.work(clean.data(), out.data());
    CHECK(a.ber() == 0.0f);

    // Uncoded random symbols: ~1/4 of the re-encoded symbols disagree => ber ~ 1
    std::vector<uint8_t> junk = randomBits(frame * 2, 5);
    std::vector<int8_t> noise = toSigned(junk, 0, frame * 2);
    viterbi::Viterbi27 b(frame, viterbi::CCSDS_R2_K7_POLYS);
    b.work(noise.data(), out.data());
    CHECK(b.ber() > 0.5f);
}

// Bug: the BER re-encoder (cc_encoder_in, frame = ber_test_size/2) keeps its shift register from the
// previous call, i.e. the state after bit 511 of the previous frame rather than the end of it.
// The first re-encoded symbols of every frame after the first are therefore wrong and ber() is
// non-zero on a perfectly clean stream (viterbi27.cpp:58).
TEST_CASE("Viterbi27 ber() stays zero on consecutive clean frames", "[common][codings][viterbi][!shouldfail]")
{
    const int frame = 2048, frames = 3;
    std::vector<uint8_t> bits = randomBits(frame * frames, 6);
    std::vector<uint8_t> syms = encode(bits);

    viterbi::Viterbi27 vit(frame, viterbi::CCSDS_R2_K7_POLYS);
    for (int f = 0; f < frames; f++)
    {
        std::vector<int8_t> soft = toSigned(syms, f * frame * 2, frame * 2);
        std::vector<uint8_t> out(frame / 8);
        vit.work(soft.data(), out.data());
        CHECK(vit.ber() == 0.0f);
    }
}
