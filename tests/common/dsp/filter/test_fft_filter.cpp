#include "common/dsp/filter/fft_filter.h"
#include "common/dsp/filter/firdes.h"
#include "../dsp_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

namespace
{
    // Block size used internally: fft = 2 * next_pow2(ntaps) * 100, nsamples = fft - ntaps + 1
    int fftFilterBlockSize(int ntaps)
    {
        int fft = (int)(2 * pow(2.0, ceil(log(double(ntaps)) / log(2.0)))) * 100;
        return fft - ntaps + 1;
    }
}

TEST_CASE("FFTFilterBlock output equals direct convolution", "[common][dsp][fft_filter]")
{
    auto taps = dsp::firdes::low_pass(1.0, 1.0, 0.15, 0.05); // 49 taps, FFT size 12800
    REQUIRE(taps.size() == 49);
    const int block = fftFilterBlockSize(taps.size());

    dsp_test::Lcg rng(20);
    std::vector<complex_t> in(3 * block + 1234);
    for (auto &x : in)
        x = complex_t(rng.uniform() * 2 - 1, rng.uniform() * 2 - 1);
    auto expected = dsp_test::referenceFir(in, taps);

    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::FFTFilterBlock<complex_t> fft(input, taps);
    auto r = dsp_test::runBlock(fft, input, in, 5000);
    REQUIRE(r.completed);

    // Only whole blocks are output, the leftover stays buffered
    REQUIRE(r.output.size() == (size_t)(3 * block));
    for (size_t i = 0; i < r.output.size(); i++)
        REQUIRE(dsp_test::absDiff(r.output[i], expected[i]) < 1e-4f);
}

TEST_CASE("FFTFilterBlock low-pass rejects an out-of-band tone", "[common][dsp][fft_filter]")
{
    auto taps = dsp::firdes::low_pass(1.0, 1.0, 0.1, 0.03, dsp::fft::window::WIN_BLACKMAN);
    const int block = fftFilterBlockSize(taps.size());

    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::FFTFilterBlock<complex_t> fft(input, taps);
    auto r = dsp_test::runBlock(fft, input, dsp_test::complexTone(2 * block + 10, 2 * M_PI * 0.3), 8192);
    REQUIRE(r.completed);
    REQUIRE(r.output.size() == (size_t)(2 * block));
    CHECK(dsp_test::toDb(dsp_test::rms(r.output, taps.size(), r.output.size())) < -60.0);

    auto input2 = std::make_shared<dsp::stream<complex_t>>();
    dsp::FFTFilterBlock<complex_t> fft2(input2, taps);
    auto r2 = dsp_test::runBlock(fft2, input2, dsp_test::complexTone(2 * block + 10, 2 * M_PI * 0.02), 8192);
    REQUIRE(r2.completed);
    CHECK(dsp_test::rms(r2.output, taps.size(), r2.output.size()) == Approx(1.0).margin(0.01));
}
