#include "common/codings/viterbi/viterbi_3_4.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <vector>

namespace
{
    const int BUFFER = 8192; // punctured soft symbols per work() call, multiple of 4
    const float THRESHOLD = 0.17f;

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

    // Rate 3/4 puncturing: every 3 input bits (X1 Y1 X2 Y2 X3 Y3) keep 4 symbols.
    // FengYun uses the standard order X1 Y1 Y2 X3, MetOp transmits X1 Y1 X3 Y2.
    std::vector<int8_t> encodePunctured(const std::vector<uint8_t> &bits, bool fengyun)
    {
        viterbi::CCEncoder enc(bits.size(), 7, 2, {79, 109});
        std::vector<uint8_t> s(bits.size() * 2);
        enc.work((uint8_t *)bits.data(), s.data());

        std::vector<int8_t> out;
        for (size_t b = 0; b + 3 <= bits.size(); b += 3)
        {
            const uint8_t *g = &s[b * 2];
            uint8_t sel[4] = {g[0], g[1], fengyun ? g[3] : g[4], fengyun ? g[4] : g[3]};
            for (uint8_t v : sel)
                out.push_back(v ? 100 : -100);
        }
        return out;
    }

    std::vector<uint8_t> run(viterbi::Viterbi3_4 &vit, const std::vector<int8_t> &stream, int offset, int buffers)
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

    int errorsAt(const std::vector<uint8_t> &decoded, const std::vector<uint8_t> &bits, int delay)
    {
        int e = 0;
        for (size_t i = 0; i < decoded.size(); i++)
            e += decoded[i] != bits[i + delay];
        return e;
    }
}

TEST_CASE("Viterbi3_4 locks and decodes clean punctured streams", "[common][codings][viterbi]")
{
    bool fengyun = GENERATE(false, true);
    // Offset in symbols; 2 symbols is half a puncturing period, detected as the alternate shift
    int offset = GENERATE(0, 2);
    INFO("fengyun " << fengyun << ", offset " << offset);

    const int buffers = 4;
    std::vector<uint8_t> bits = randomBits(BUFFER * buffers, 1);
    std::vector<int8_t> stream = encodePunctured(bits, fengyun);

    viterbi::Viterbi3_4 vit(THRESHOLD, 5, BUFFER, fengyun);
    std::vector<uint8_t> decoded = run(vit, stream, offset, buffers);

    CHECK(vit.getState() == 1);
    CHECK(vit.getshift() == (offset == 2));
    CHECK(vit.ber() < THRESHOLD);
    // 4 punctured symbols carry 3 bits
    REQUIRE(decoded.size() == size_t(BUFFER * 3 / 4 * buffers));
    // Starting 2 symbols in, X1 Y1 are skipped so the output starts at bit 1
    CHECK(errorsAt(decoded, bits, offset == 2 ? 1 : 0) == 0);
}

TEST_CASE("Viterbi3_4 MetOp mode resolves a 90 degree phase rotation", "[common][codings][viterbi]")
{
    const int buffers = 3;
    std::vector<uint8_t> bits = randomBits(BUFFER * buffers, 2);
    std::vector<int8_t> stream = encodePunctured(bits, false);
    // The decoder applies PHASE_90 as its alternate phase, so pre-rotate by the inverse
    rotate_soft(stream.data(), stream.size(), PHASE_270, false);

    viterbi::Viterbi3_4 vit(THRESHOLD, 5, BUFFER, false);
    std::vector<uint8_t> decoded = run(vit, stream, 0, buffers);

    CHECK(vit.getState() == 1);
    REQUIRE(decoded.size() == size_t(BUFFER * 3 / 4 * buffers));
    CHECK(errorsAt(decoded, bits, 0) == 0);
}

TEST_CASE("Viterbi3_4 FengYun mode does not search alternate phases", "[common][codings][viterbi]")
{
    std::vector<uint8_t> bits = randomBits(BUFFER * 2, 3);
    std::vector<int8_t> stream = encodePunctured(bits, true);
    rotate_soft(stream.data(), stream.size(), PHASE_270, false);

    viterbi::Viterbi3_4 vit(THRESHOLD, 5, BUFFER, true);
    run(vit, stream, 0, 1);
    CHECK(vit.getState() == 0);
}

TEST_CASE("Viterbi3_4 corrects channel errors while locked", "[common][codings][viterbi]")
{
    const int buffers = 3;
    std::vector<uint8_t> bits = randomBits(BUFFER * buffers, 4);
    std::vector<int8_t> stream = encodePunctured(bits, false);
    Lcg rng(9);
    for (auto &s : stream)
        if (rng.below(1000) < 5) // 0.5% hard flips, rate 3/4 has a much smaller free distance
            s = -s;

    viterbi::Viterbi3_4 vit(THRESHOLD, 5, BUFFER, false);
    std::vector<uint8_t> decoded = run(vit, stream, 0, buffers);

    CHECK(vit.getState() == 1);
    REQUIRE(decoded.size() == size_t(BUFFER * 3 / 4 * buffers));
    CHECK(errorsAt(decoded, bits, 0) <= 10);
}

TEST_CASE("Viterbi3_4 does not lock on noise, unlocks after max_outsync, and reset() unlocks", "[common][codings][viterbi]")
{
    const int maxOutsync = 1;
    std::vector<uint8_t> junk = randomBits(BUFFER * 4, 5);
    std::vector<int8_t> noise(junk.size());
    for (size_t i = 0; i < junk.size(); i++)
        noise[i] = junk[i] ? 100 : -100;

    std::vector<uint8_t> bits = randomBits(BUFFER * 2, 6);
    std::vector<int8_t> clean = encodePunctured(bits, false);

    viterbi::Viterbi3_4 vit(THRESHOLD, maxOutsync, BUFFER, false);
    std::vector<uint8_t> out(BUFFER);

    std::vector<int8_t> in(noise.begin(), noise.begin() + BUFFER);
    CHECK(vit.work(in.data(), BUFFER, out.data()) == 0);
    CHECK(vit.getState() == 0);
    CHECK(vit.ber() > THRESHOLD);

    in.assign(clean.begin(), clean.begin() + BUFFER);
    CHECK(vit.work(in.data(), BUFFER, out.data()) == BUFFER * 3 / 4);
    REQUIRE(vit.getState() == 1);

    for (int i = 0; i <= maxOutsync; i++)
    {
        CHECK(vit.getState() == 1);
        in.assign(noise.begin() + (i + 1) * BUFFER, noise.begin() + (i + 2) * BUFFER);
        vit.work(in.data(), BUFFER, out.data());
    }
    CHECK(vit.getState() == 0);

    in.assign(clean.begin(), clean.begin() + BUFFER);
    vit.work(in.data(), BUFFER, out.data());
    REQUIRE(vit.getState() == 1);
    vit.reset();
    CHECK(vit.getState() == 0);
}
