#include "common/dsp/demod/pm_to_bpsk.h"
#include "../dsp_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

TEST_CASE("PMToBPSK keeps only Q and shifts it down by the symbol rate", "[common][dsp][pm_to_bpsk]")
{
    const float fs = 100000, rs = 5000;
    const double w = dsp::hz_to_rad(rs, fs);

    // I is discarded entirely, Q is kept
    dsp_test::Lcg rng(30);
    std::vector<complex_t> in(20000);
    for (auto &x : in)
        x = complex_t(5.0f + rng.uniform(), 0.25f + 0.5f * rng.uniform());

    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::PMToBPSK blk(input, fs, rs);
    auto r = dsp_test::runBlock(blk, input, in, 3000);
    REQUIRE(r.completed);
    REQUIRE(r.output.size() == in.size());

    // out[n] = j * Q[n] * exp(-j w n)
    for (size_t n = 0; n < in.size(); n++)
    {
        double p = -w * (double)n;
        complex_t e(-in[n].imag * sin(p), in[n].imag * cos(p));
        REQUIRE(dsp_test::absDiff(r.output[n], e) < 2e-3f);
    }
}

TEST_CASE("PMToBPSK brings a Q-channel subcarrier at the symbol rate to DC", "[common][dsp][pm_to_bpsk]")
{
    const float fs = 64000, rs = 4000;
    const double w = dsp::hz_to_rad(rs, fs);

    // Q = cos(w n): after the shift, half the energy is at DC and half at -2w
    std::vector<complex_t> in(32000);
    for (size_t n = 0; n < in.size(); n++)
        in[n] = complex_t(1.0f, cos(w * n));

    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::PMToBPSK blk(input, fs, rs);
    auto r = dsp_test::runBlock(blk, input, in, 4096);
    REQUIRE(r.completed);

    // Averaging over whole periods leaves the DC term j / 2
    std::complex<double> mean = 0;
    for (size_t n = 0; n < 32000; n++)
        mean += std::complex<double>(r.output[n].real, r.output[n].imag);
    mean /= 32000.0;
    CHECK(mean.real() == Approx(0.0).margin(1e-3));
    CHECK(mean.imag() == Approx(0.5).margin(1e-3));
}
