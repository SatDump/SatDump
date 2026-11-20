#include "common/codings/viterbi/viterbi_1_2.h"
#include "helpers/random.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <vector>

namespace
{
    const int BUFFER = 8192; // soft symbols per work() call
    const float THRESHOLD = 0.17f;

    using test_helpers::Lcg;
    using test_helpers::randomBits;

    std::vector<int8_t> encodeSigned(const std::vector<uint8_t> &bits)
    {
        viterbi::CCEncoder enc(bits.size(), 7, 2, {79, 109});
        std::vector<uint8_t> syms(bits.size() * 2);
        enc.work((uint8_t *)bits.data(), syms.data());
        std::vector<int8_t> soft(syms.size());
        for (size_t i = 0; i < syms.size(); i++)
            soft[i] = syms[i] ? 100 : -100;
        return soft;
    }

    // Feed `buffers` consecutive buffers starting at `offset` and collect the decoded bits.
    // work() rotates its input in place, hence the copy.
    std::vector<uint8_t> run(viterbi::Viterbi1_2 &vit, const std::vector<int8_t> &stream, int offset, int buffers)
    {
        std::vector<uint8_t> decoded;
        std::vector<uint8_t> out(BUFFER);
        for (int b = 0; b < buffers; b++)
        {
            std::vector<int8_t> in(stream.begin() + offset + b * BUFFER, stream.begin() + offset + (b + 1) * BUFFER);
            int n = vit.work(in.data(), BUFFER, out.data());
            decoded.insert(decoded.end(), out.begin(), out.begin() + n);
        }
        return decoded;
    }

    // Errors of `decoded` against `bits` delayed by `delay`, taking the best of both polarities
    int alignedErrors(const std::vector<uint8_t> &decoded, const std::vector<uint8_t> &bits, int delay)
    {
        int direct = 0, inverted = 0;
        for (size_t i = 0; i < decoded.size(); i++)
        {
            direct += decoded[i] != bits[i + delay];
            inverted += decoded[i] != (bits[i + delay] ^ 1);
        }
        return std::min(direct, inverted);
    }
} // namespace

TEST_CASE("Viterbi1_2 locks and decodes a clean aligned stream", "[common][codings][viterbi]")
{
    const int buffers = 4;
    std::vector<uint8_t> bits = randomBits(BUFFER / 2 * buffers + 64, 1);
    std::vector<int8_t> stream = encodeSigned(bits);

    viterbi::Viterbi1_2 vit(THRESHOLD, 5, BUFFER);
    CHECK(vit.getState() == 0);
    std::vector<uint8_t> decoded = run(vit, stream, 0, buffers);

    CHECK(vit.getState() == 1);
    CHECK_FALSE(vit.getshift());
    CHECK(vit.ber() < THRESHOLD);
    REQUIRE(decoded.size() == size_t(BUFFER / 2 * buffers));
    CHECK(std::equal(decoded.begin(), decoded.end(), bits.begin()));
}

TEST_CASE("Viterbi1_2 resolves phase rotations up to the 180 degree data inversion", "[common][codings][viterbi]")
{
    auto phase = GENERATE(PHASE_0, PHASE_90, PHASE_180, PHASE_270);
    INFO("phase " << phase);
    const int buffers = 3;
    std::vector<uint8_t> bits = randomBits(BUFFER / 2 * buffers + 64, 2);
    std::vector<int8_t> stream = encodeSigned(bits);
    rotate_soft(stream.data(), stream.size(), phase, false);

    viterbi::Viterbi1_2 vit(THRESHOLD, 5, BUFFER);
    std::vector<uint8_t> decoded = run(vit, stream, 0, buffers);

    CHECK(vit.getState() == 1);
    REQUIRE(decoded.size() == size_t(BUFFER / 2 * buffers));
    // Both generators have odd weight, so a 180 degree rotation decodes to the complemented data
    CHECK(alignedErrors(decoded, bits, 0) == 0);
}

TEST_CASE("Viterbi1_2 locks onto a stream that is one symbol out of alignment", "[common][codings][viterbi]")
{
    const int buffers = 3;
    std::vector<uint8_t> bits = randomBits(BUFFER / 2 * buffers + 64, 3);
    std::vector<int8_t> stream = encodeSigned(bits);

    viterbi::Viterbi1_2 vit(THRESHOLD, 5, BUFFER);
    std::vector<uint8_t> decoded = run(vit, stream, 1, buffers); // starts at the Y symbol of bit 0

    CHECK(vit.getState() == 1);
    CHECK(vit.getshift());
    REQUIRE(decoded.size() == size_t(BUFFER / 2 * buffers));
    // Bit 0 cannot be recovered, so the output starts at bit 1
    CHECK(alignedErrors(decoded, bits, 1) == 0);
}

TEST_CASE("Viterbi1_2 handles IQ swap when enabled", "[common][codings][viterbi]")
{
    const int buffers = 3;
    std::vector<uint8_t> bits = randomBits(BUFFER / 2 * buffers + 64, 4);
    std::vector<int8_t> stream = encodeSigned(bits);
    rotate_soft(stream.data(), stream.size(), PHASE_0, true);

    viterbi::Viterbi1_2 vit(THRESHOLD, 5, BUFFER, {PHASE_0}, true);
    std::vector<uint8_t> decoded = run(vit, stream, 0, buffers);

    CHECK(vit.getState() == 1);
    REQUIRE(decoded.size() == size_t(BUFFER / 2 * buffers));
    CHECK(alignedErrors(decoded, bits, 0) == 0);
}

TEST_CASE("Viterbi1_2 corrects channel errors while locked", "[common][codings][viterbi]")
{
    const int buffers = 4;
    std::vector<uint8_t> bits = randomBits(BUFFER / 2 * buffers + 64, 5);
    std::vector<int8_t> stream = encodeSigned(bits);
    Lcg rng(77);
    for (auto &s : stream)
        if (rng.below(100) < 2) // 2% hard flips
            s = -s;

    viterbi::Viterbi1_2 vit(THRESHOLD, 5, BUFFER);
    std::vector<uint8_t> decoded = run(vit, stream, 0, buffers);

    CHECK(vit.getState() == 1);
    CHECK(vit.ber() > 0.0f);
    CHECK(vit.ber() < THRESHOLD);
    REQUIRE(decoded.size() == size_t(BUFFER / 2 * buffers));
    CHECK(alignedErrors(decoded, bits, 0) <= 5);
}

TEST_CASE("Viterbi1_2 does not lock on noise and loses lock after max_outsync bad buffers", "[common][codings][viterbi]")
{
    const int maxOutsync = 2;
    std::vector<uint8_t> junkBits = randomBits(BUFFER * 8, 6);
    std::vector<int8_t> noise(junkBits.size());
    for (size_t i = 0; i < junkBits.size(); i++)
        noise[i] = junkBits[i] ? 100 : -100;

    viterbi::Viterbi1_2 vit(THRESHOLD, maxOutsync, BUFFER);
    std::vector<uint8_t> out(BUFFER);

    // Noise: never locks, produces nothing
    {
        std::vector<int8_t> in(noise.begin(), noise.begin() + BUFFER);
        CHECK(vit.work(in.data(), BUFFER, out.data()) == 0);
        CHECK(vit.getState() == 0);
        CHECK(vit.ber() > THRESHOLD);
    }

    // Lock on a clean buffer
    std::vector<uint8_t> bits = randomBits(BUFFER / 2 + 64, 7);
    std::vector<int8_t> clean = encodeSigned(bits);
    clean.resize(BUFFER);
    CHECK(vit.work(clean.data(), BUFFER, out.data()) == BUFFER / 2);
    REQUIRE(vit.getState() == 1);

    // Stays locked (still outputting) for max_outsync bad buffers, unlocks on the next one
    for (int i = 0; i <= maxOutsync; i++)
    {
        INFO("bad buffer " << i);
        CHECK(vit.getState() == 1);
        std::vector<int8_t> in(noise.begin() + (i + 1) * BUFFER, noise.begin() + (i + 2) * BUFFER);
        CHECK(vit.work(in.data(), BUFFER, out.data()) == BUFFER / 2);
    }
    CHECK(vit.getState() == 0);
}

// Bug: the BER re-encoder (cc_encoder_ber, frame = 1024 bits) keeps its register state from the
// previous re-encode, which is the state 1024 bits into the previous buffer rather than at its end,
// so ber() is non-zero on a perfectly clean stream (viterbi_1_2.cpp:101).
TEST_CASE("Viterbi1_2 ber() is zero on a clean locked stream", "[common][codings][viterbi][!shouldfail]")
{
    const int buffers = 3;
    std::vector<uint8_t> bits = randomBits(BUFFER / 2 * buffers + 64, 8);
    std::vector<int8_t> stream = encodeSigned(bits);
    viterbi::Viterbi1_2 vit(THRESHOLD, 5, BUFFER);
    run(vit, stream, 0, buffers);
    CHECK(vit.ber() == 0.0f);
}
