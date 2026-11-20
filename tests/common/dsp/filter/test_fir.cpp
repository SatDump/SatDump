#include "common/dsp/dsp_test_helpers.h"
#include "common/dsp/filter/fir.h"
#include "common/dsp/filter/firdes.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

namespace
{
    std::vector<float> randomReal(size_t n, uint32_t seed)
    {
        dsp_test::Lcg rng(seed);
        std::vector<float> v(n);
        for (auto &x : v)
            x = rng.uniform() * 2 - 1;
        return v;
    }

    std::vector<complex_t> randomComplex(size_t n, uint32_t seed)
    {
        dsp_test::Lcg rng(seed);
        std::vector<complex_t> v(n);
        for (auto &x : v)
            x = complex_t(rng.uniform() * 2 - 1, rng.uniform() * 2 - 1);
        return v;
    }

    template <typename T>
    std::vector<T> runFir(const std::vector<T> &in, const std::vector<float> &taps, int chunk)
    {
        auto input = std::make_shared<dsp::stream<T>>();
        dsp::FIRBlock<T> fir(input, taps);
        auto r = dsp_test::runBlock(fir, input, in, chunk);
        REQUIRE(r.completed);
        return r.output;
    }
} // namespace

TEST_CASE("FIRBlock impulse response equals the taps", "[common][dsp][fir]")
{
    std::vector<float> taps = {0.1f, -0.2f, 0.3f, 0.5f, -0.7f, 0.11f, 0.13f};
    std::vector<float> impulse(64, 0.0f);
    impulse[0] = 1.0f;

    auto out = runFir(impulse, taps, 64);
    REQUIRE(out.size() == impulse.size());
    for (size_t i = 0; i < taps.size(); i++)
        CHECK(out[i] == Approx(taps[i]).margin(1e-6));
    for (size_t i = taps.size(); i < out.size(); i++)
        CHECK(out[i] == Approx(0.0f).margin(1e-6));
}

TEST_CASE("FIRBlock<float> matches direct convolution across chunk boundaries", "[common][dsp][fir]")
{
    auto taps = dsp::firdes::low_pass(1.0, 1.0, 0.2, 0.05);
    auto in = randomReal(20000, 1);
    auto expected = dsp_test::referenceFir(in, taps);

    // Odd chunk sizes stress history handling and misaligned dot products
    for (int chunk : {1, 333, 4096, 20000})
    {
        INFO("chunk = " << chunk);
        auto out = runFir(in, taps, chunk);
        REQUIRE(out.size() == in.size());
        size_t mismatches = 0, first = 0;
        for (size_t i = 0; i < in.size(); i++)
            if (!(out[i] == Approx(expected[i]).margin(1e-5)) && mismatches++ == 0)
                first = i;
        INFO("first mismatch at sample " << first);
        REQUIRE(mismatches == 0);
    }
}

TEST_CASE("FIRBlock<complex_t> matches direct convolution across chunk boundaries", "[common][dsp][fir]")
{
    auto taps = dsp::firdes::root_raised_cosine(1.0, 4, 1, 0.35, 31);
    auto in = randomComplex(15000, 2);
    auto expected = dsp_test::referenceFir(in, taps);

    for (int chunk : {7, 1000, 15000})
    {
        INFO("chunk = " << chunk);
        auto out = runFir(in, taps, chunk);
        REQUIRE(out.size() == in.size());
        size_t mismatches = 0, first = 0;
        for (size_t i = 0; i < in.size(); i++)
            if (!(dsp_test::absDiff(out[i], expected[i]) < 1e-5f) && mismatches++ == 0)
                first = i;
        INFO("first mismatch at sample " << first);
        REQUIRE(mismatches == 0);
    }
}

TEST_CASE("FIRBlock low-pass passes in-band and rejects out-of-band tones", "[common][dsp][fir]")
{
    const double fs = 48000;
    auto taps = dsp::firdes::low_pass(1.0, fs, 5000, 1000, dsp::fft::window::WIN_BLACKMAN);
    const size_t n = 20000;

    auto pass = runFir(dsp_test::complexTone(n, dsp::hz_to_rad(2000, fs)), taps, 4096);
    auto stop = runFir(dsp_test::complexTone(n, dsp::hz_to_rad(9000, fs)), taps, 4096);

    // Skip the filter transient
    size_t s = taps.size();
    CHECK(dsp_test::rms(pass, s, n) == Approx(1.0).margin(0.01));
    CHECK(dsp_test::toDb(dsp_test::rms(stop, s, n)) < -60.0);
    // Tone frequency is unchanged by filtering
    CHECK(dsp_test::estimateFreq(pass, s, n) == Approx(dsp::hz_to_rad(2000, fs)).epsilon(1e-4));
}

TEST_CASE("FIRBlock with a single tap is a gain", "[common][dsp][fir]")
{
    auto in = randomReal(5000, 3);
    auto out = runFir(in, {-2.0f}, 1024);
    REQUIRE(out.size() == in.size());
    size_t mismatches = 0, first = 0;
    for (size_t i = 0; i < in.size(); i++)
        if (!(out[i] == Approx(-2.0f * in[i]).margin(1e-6)) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}
