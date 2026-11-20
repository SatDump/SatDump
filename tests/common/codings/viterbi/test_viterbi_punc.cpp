#include "common/codings/viterbi/viterbi_punc.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <string>
#include <vector>

namespace
{
    const int BUFFER = 8192; // received soft symbols per work() call
    const float THRESHOLD = 0.17f;

    struct Rate
    {
        std::string name;
        std::string x, y; // DVB-S puncturing pattern
        double r;
    };

    std::shared_ptr<viterbi::puncturing::GenericDepunc> makeDepunc(const std::string &name)
    {
        using namespace viterbi::puncturing;
        if (name == "2/3")
            return std::make_shared<Depunc23>();
        if (name == "3/4")
            return std::make_shared<Depunc34>();
        if (name == "5/6")
            return std::make_shared<Depunc56>();
        return std::make_shared<Depunc78>();
    }

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

    std::vector<int8_t> encodePunctured(const std::vector<uint8_t> &bits, const Rate &rate)
    {
        viterbi::CCEncoder enc(bits.size(), 7, 2, {79, 109});
        std::vector<uint8_t> s(bits.size() * 2);
        enc.work((uint8_t *)bits.data(), s.data());
        std::vector<int8_t> out;
        int period = rate.x.size();
        for (size_t b = 0; b < bits.size(); b++)
        {
            if (rate.x[b % period] == '1')
                out.push_back(s[b * 2] ? 100 : -100);
            if (rate.y[b % period] == '1')
                out.push_back(s[b * 2 + 1] ? 100 : -100);
        }
        return out;
    }

    std::vector<uint8_t> run(viterbi::Viterbi_Depunc &vit, const std::vector<int8_t> &stream, int offset, int buffers)
    {
        std::vector<uint8_t> decoded;
        std::vector<uint8_t> out(BUFFER * 8);
        for (int b = 0; b < buffers; b++)
        {
            std::vector<int8_t> in(stream.begin() + offset + b * BUFFER, stream.begin() + offset + (b + 1) * BUFFER);
            int n = vit.work(in.data(), BUFFER, out.data());
            decoded.insert(decoded.end(), out.begin(), out.begin() + n);
        }
        return decoded;
    }

    int errorsAt(const std::vector<uint8_t> &decoded, const std::vector<uint8_t> &bits, int delay, int polarity)
    {
        int e = 0;
        for (size_t i = 0; i < decoded.size(); i++)
            e += decoded[i] != (bits[i + delay] ^ polarity);
        return e;
    }
}

TEST_CASE("Viterbi_Depunc locks onto and decodes every DVB-S punctured rate", "[common][codings][viterbi]")
{
    auto rate = GENERATE(Rate{"2/3", "10", "11", 2.0 / 3.0}, Rate{"3/4", "101", "110", 3.0 / 4.0},
                         Rate{"5/6", "10101", "11010", 5.0 / 6.0}, Rate{"7/8", "1000101", "1111010", 7.0 / 8.0});
    int offset = GENERATE(0, 1, 2);
    INFO("rate " << rate.name << ", offset " << offset);

    const int buffers = 5;
    std::vector<uint8_t> bits = randomBits(BUFFER * buffers, 1);
    std::vector<int8_t> stream = encodePunctured(bits, rate);

    viterbi::Viterbi_Depunc vit(makeDepunc(rate.name), THRESHOLD, 5, BUFFER);
    std::vector<uint8_t> decoded = run(vit, stream, offset, buffers);

    CHECK(vit.getState() == 1);
    CHECK(vit.ber() < THRESHOLD);

    // Output comes in chunks of BUFFER/2 bits, the remainder stays buffered
    double expectedBits = BUFFER * buffers * rate.r;
    CHECK(decoded.size() <= expectedBits);
    CHECK(decoded.size() + BUFFER / 2 >= expectedBits);
    CHECK(decoded.size() % (BUFFER / 2) == 0);

    if (offset == 0)
    {
        CHECK_FALSE(vit.getshift());
        CHECK(errorsAt(decoded, bits, 0, 0) == 0);
    }
    else
    {
        // Starting mid-period the first bit(s) are skipped; which shift locks first decides the
        // alignment (and for some, a complemented equivalent path), so accept any small delay
        int best = int(decoded.size());
        for (int delay = 0; delay <= 2; delay++)
            for (int pol = 0; pol < 2; pol++)
                best = std::min(best, errorsAt(decoded, bits, delay, pol));
        CHECK(best == 0);
    }
}

TEST_CASE("Viterbi_Depunc resolves a 90 degree rotation", "[common][codings][viterbi]")
{
    Rate rate{"3/4", "101", "110", 0.75};
    const int buffers = 3;
    std::vector<uint8_t> bits = randomBits(BUFFER * buffers, 2);
    std::vector<int8_t> stream = encodePunctured(bits, rate);
    rotate_soft(stream.data(), stream.size(), PHASE_270, false);

    viterbi::Viterbi_Depunc vit(makeDepunc(rate.name), THRESHOLD, 5, BUFFER);
    std::vector<uint8_t> decoded = run(vit, stream, 0, buffers);

    CHECK(vit.getState() == 1);
    REQUIRE(!decoded.empty());
    CHECK(std::min(errorsAt(decoded, bits, 0, 0), errorsAt(decoded, bits, 0, 1)) == 0);
}

TEST_CASE("Viterbi_Depunc does not lock on noise and loses lock after max_outsync", "[common][codings][viterbi]")
{
    Rate rate{"2/3", "10", "11", 2.0 / 3.0};
    const int maxOutsync = 2;
    std::vector<uint8_t> junk = randomBits(BUFFER * 4, 3);
    std::vector<int8_t> noise(junk.size());
    for (size_t i = 0; i < junk.size(); i++)
        noise[i] = junk[i] ? 100 : -100;

    viterbi::Viterbi_Depunc vit(makeDepunc(rate.name), THRESHOLD, maxOutsync, BUFFER);
    std::vector<uint8_t> out(BUFFER * 8);

    std::vector<int8_t> in(noise.begin(), noise.begin() + BUFFER);
    CHECK(vit.work(in.data(), BUFFER, out.data()) == 0);
    CHECK(vit.getState() == 0);
    CHECK(vit.ber() > THRESHOLD);

    std::vector<uint8_t> bits = randomBits(BUFFER * 2, 4);
    std::vector<int8_t> clean = encodePunctured(bits, rate);
    in.assign(clean.begin(), clean.begin() + BUFFER);
    vit.work(in.data(), BUFFER, out.data());
    REQUIRE(vit.getState() == 1);

    // Leftover clean symbols in the sliding buffer may make the first noisy chunk look fine, so only
    // require staying locked for max_outsync buffers and unlocking a few buffers later
    int noiseBuffers = 0;
    while (vit.getState() == 1 && noiseBuffers < 6)
    {
        if (noiseBuffers == maxOutsync)
            CHECK(vit.getState() == 1);
        in.assign(noise.begin() + (noiseBuffers % 3 + 1) * BUFFER, noise.begin() + (noiseBuffers % 3 + 2) * BUFFER);
        vit.work(in.data(), BUFFER, out.data());
        noiseBuffers++;
    }
    CHECK(vit.getState() == 0);
    CHECK(noiseBuffers > maxOutsync);
    CHECK(noiseBuffers <= maxOutsync + 2);
}
