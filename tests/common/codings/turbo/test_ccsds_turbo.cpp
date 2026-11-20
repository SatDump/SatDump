#include "common/codings/turbo/ccsds_turbo.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace
{
    using codings::turbo::CCSDSTurbo;
    using codings::turbo::turbo_base_t;
    using codings::turbo::turbo_rate_t;

    struct Lcg
    {
        uint32_t state;
        explicit Lcg(uint32_t seed) : state(seed) {}
        uint32_t next()
        {
            state = state * 1664525u + 1013904223u;
            return state;
        }
        double uniform() { return ((next() >> 8) + 0.5) / 16777216.0; }
        double gaussian()
        {
            double u1 = uniform(), u2 = uniform();
            return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2);
        }
    };

    std::vector<uint8_t> randomBytes(int count, uint32_t seed)
    {
        Lcg rng(seed);
        std::vector<uint8_t> v(count);
        for (auto &b : v)
            b = rng.next() >> 24;
        return v;
    }

    // CCSDS 131.0-B section 6 turbo interleaver (k1 = 8, k2 = info bytes)
    std::vector<int> ccsdsPermutation(int k2)
    {
        const int k1 = 8;
        const int p[8] = {31, 37, 43, 47, 53, 59, 61, 67};
        int k = k1 * k2;
        std::vector<int> pi(k);
        for (int s = 1; s <= k; s++)
        {
            int m = (s - 1) % 2;
            int i = (s - 1) / (2 * k2);
            int j = (s - 1) / 2 - i * k2;
            int t = (19 * i + 1) % (k1 / 2);
            int q = t % 8 + 1;
            int c = (p[q - 1] * j + 21 * m) % k2;
            pi[s - 1] = 2 * (t + c * (k1 / 2) + 1) - m - 1; // 0-based index of the bit read at time s
        }
        return pi;
    }

    // One CCSDS recursive systematic component encoder: feedback G0 = 1 + D^3 + D^4,
    // outputs given as tap strings (index = delay). The 4 tail steps drive the register to 0.
    std::vector<std::vector<uint8_t>> componentEncode(const std::vector<uint8_t> &u, const std::vector<std::string> &taps)
    {
        std::vector<std::vector<uint8_t>> out(taps.size());
        uint8_t w[5] = {0}; // w[d] = register value delayed by d
        for (size_t t = 0; t < u.size() + 4; t++)
        {
            uint8_t fb = w[2] ^ w[3]; // w_{t-3} ^ w_{t-4}, before shifting
            for (int d = 4; d > 0; d--)
                w[d] = w[d - 1];
            w[0] = t < u.size() ? (u[t] ^ fb) : 0;
            for (size_t o = 0; o < taps.size(); o++)
            {
                uint8_t v = 0;
                for (int d = 0; d < 5; d++)
                    if (taps[o][d] == '1')
                        v ^= w[d];
                out[o].push_back(v);
            }
        }
        return out;
    }

    struct RateDef
    {
        turbo_rate_t rate;
        std::vector<std::string> upper, lower;
        bool punctured;
        int inverse;
    };

    RateDef rateDef(turbo_rate_t r)
    {
        switch (r)
        {
        case codings::turbo::RATE_1_2:
            return {r, {"10011", "11011"}, {"11011"}, true, 2};
        case codings::turbo::RATE_1_3:
            return {r, {"10011", "11011"}, {"11011"}, false, 3};
        case codings::turbo::RATE_1_4:
            return {r, {"10011", "10101", "11111"}, {"11011"}, false, 4};
        default:
            return {r, {"10011", "11011", "10101", "11111"}, {"11011", "11111"}, false, 6};
        }
    }

    // Independent reference of the CCSDS turbo encoder output bit sequence
    std::vector<uint8_t> referenceEncode(const std::vector<uint8_t> &frame, turbo_rate_t rate)
    {
        RateDef def = rateDef(rate);
        int k = frame.size() * 8;
        std::vector<uint8_t> u(k);
        for (int i = 0; i < k; i++)
            u[i] = (frame[i / 8] >> (7 - i % 8)) & 1;
        std::vector<int> pi = ccsdsPermutation(frame.size());
        std::vector<uint8_t> ui(k);
        for (int i = 0; i < k; i++)
            ui[i] = u[pi[i]];

        auto a = componentEncode(u, def.upper);
        auto b = componentEncode(ui, def.lower);

        std::vector<uint8_t> out;
        int sym = 0;
        for (int t = 0; t < k + 4; t++)
        {
            std::vector<uint8_t> step;
            for (auto &o : a)
                step.push_back(o[t]);
            for (auto &o : b)
                step.push_back(o[t]);
            if (def.punctured)
            {
                // Keep the systematic bit, alternate between the upper and lower parity
                out.push_back(step[0]);
                out.push_back(sym % 2 ? step[2] : step[1]);
                sym++;
            }
            else
                out.insert(out.end(), step.begin(), step.end());
        }
        return out;
    }

    std::vector<uint8_t> unpack(const std::vector<uint8_t> &bytes, int bits)
    {
        std::vector<uint8_t> out(bits);
        for (int i = 0; i < bits; i++)
            out[i] = (bytes[i / 8] >> (7 - i % 8)) & 1;
        return out;
    }

    std::vector<float> toBpsk(const std::vector<uint8_t> &bits)
    {
        std::vector<float> out(bits.size());
        for (size_t i = 0; i < bits.size(); i++)
            out[i] = bits[i] ? 1.0f : -1.0f;
        return out;
    }
}

TEST_CASE("CCSDSTurbo frame and codeword lengths for all bases and rates", "[common][codings][turbo]")
{
    for (turbo_base_t base : {codings::turbo::BASE_223, codings::turbo::BASE_446, codings::turbo::BASE_892, codings::turbo::BASE_1115})
    {
        for (turbo_rate_t rate : {codings::turbo::RATE_1_2, codings::turbo::RATE_1_3, codings::turbo::RATE_1_4, codings::turbo::RATE_1_6})
        {
            INFO("base " << int(base) << ", rate " << int(rate));
            CCSDSTurbo turbo(base, rate);
            int k = int(base) * 8;
            CHECK(turbo.frame_length() == k);
            // CCSDS: n = (k + 4) / r, 4 tail bits
            CHECK(turbo.codeword_length() == (k + 4) * rateDef(rate).inverse);
        }
    }
}

TEST_CASE("CCSDSTurbo permutation reference is a bijection", "[common][codings][turbo]")
{
    // Sanity check for the test's own reference interleaver
    for (int k2 : {223, 446, 892, 1115})
    {
        std::vector<int> pi = ccsdsPermutation(k2);
        std::vector<bool> seen(pi.size(), false);
        for (int v : pi)
        {
            REQUIRE(v >= 0);
            REQUIRE(v < int(pi.size()));
            seen[v] = true;
        }
        CHECK(std::all_of(seen.begin(), seen.end(), [](bool b) { return b; }));
    }
}

TEST_CASE("CCSDSTurbo::encode matches the CCSDS reference encoder", "[common][codings][turbo]")
{
    auto rate = GENERATE(codings::turbo::RATE_1_2, codings::turbo::RATE_1_3, codings::turbo::RATE_1_4, codings::turbo::RATE_1_6);
    auto base = GENERATE(codings::turbo::BASE_223, codings::turbo::BASE_1115);
    INFO("base " << int(base) << ", rate " << int(rate));

    CCSDSTurbo turbo(base, rate);
    std::vector<uint8_t> frame = randomBytes(int(base), int(base) + int(rate));
    std::vector<uint8_t> codeword((turbo.codeword_length() + 7) / 8, 0);
    turbo.encode(frame.data(), codeword.data());

    std::vector<uint8_t> expected = referenceEncode(frame, rate);
    REQUIRE(int(expected.size()) == turbo.codeword_length());

    // Compare only the full bytes, see the separate test for the trailing partial byte
    int fullBits = turbo.codeword_length() / 8 * 8;
    std::vector<uint8_t> got = unpack(codeword, fullBits);
    CHECK(std::equal(got.begin(), got.end(), expected.begin()));
}

TEST_CASE("CCSDSTurbo encode/decode round-trip on clean symbols", "[common][codings][turbo]")
{
    auto rate = GENERATE(codings::turbo::RATE_1_2, codings::turbo::RATE_1_3, codings::turbo::RATE_1_4, codings::turbo::RATE_1_6);
    INFO("rate " << int(rate));

    CCSDSTurbo turbo(codings::turbo::BASE_223, rate);
    std::vector<uint8_t> frame = randomBytes(223, 10 + int(rate));
    // Soft input built from the reference bit sequence (+1 = bit 1)
    std::vector<float> soft = toBpsk(referenceEncode(frame, rate));

    std::vector<uint8_t> decoded(223, 0);
    turbo.decode(soft.data(), decoded.data(), 2);
    CHECK(decoded == frame);
}

TEST_CASE("CCSDSTurbo decodes through AWGN", "[common][codings][turbo]")
{
    // Eb/N0 around 2 dB for every rate: sigma^2 = 1 / (2 * R * Eb/N0)
    auto rate = GENERATE(codings::turbo::RATE_1_2, codings::turbo::RATE_1_3, codings::turbo::RATE_1_6);
    INFO("rate " << int(rate));
    double r = 1.0 / rateDef(rate).inverse;
    double sigma = std::sqrt(1.0 / (2.0 * r * std::pow(10.0, 2.0 / 10.0)));

    CCSDSTurbo turbo(codings::turbo::BASE_223, rate);
    turbo.set_sigma(sigma);
    std::vector<uint8_t> frame = randomBytes(223, 20 + int(rate));
    std::vector<float> soft = toBpsk(referenceEncode(frame, rate));

    Lcg rng(1000 + int(rate));
    int hardErrors = 0;
    for (auto &s : soft)
    {
        float clean = s;
        s += sigma * rng.gaussian();
        hardErrors += (s > 0) != (clean > 0);
    }
    REQUIRE(hardErrors > 50); // the channel really is noisy

    std::vector<uint8_t> decoded(223, 0);
    turbo.decode(soft.data(), decoded.data(), 8);
    CHECK(decoded == frame);
}

// BUG: encode() only clears codeword_length()/8 bytes, so when the codeword length is not a multiple
// of 8 (every rate 1/3 base: 3 * (k + 4) bits) the last partial byte keeps stale bits from the caller's
// buffer shifted into its upper half (ccsds_turbo.cpp:150).
TEST_CASE("CCSDSTurbo::encode output does not depend on the previous buffer contents", "[common][codings][turbo][!shouldfail]")
{
    CCSDSTurbo turbo(codings::turbo::BASE_223, codings::turbo::RATE_1_3);
    REQUIRE(turbo.codeword_length() % 8 != 0);
    std::vector<uint8_t> frame = randomBytes(223, 40);
    int bytes = (turbo.codeword_length() + 7) / 8;
    std::vector<uint8_t> a(bytes, 0x00), b(bytes, 0xFF);
    turbo.encode(frame.data(), a.data());
    turbo.encode(frame.data(), b.data());
    CHECK(std::equal(a.begin(), a.end() - 1, b.begin()));
    CHECK(int(a.back()) == int(b.back()));
}
