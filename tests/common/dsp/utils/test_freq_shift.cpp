#include "common/dsp/utils/freq_shift.h"
#include "../dsp_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

TEST_CASE("FreqShiftBlock shifts a tone by the requested frequency", "[common][dsp][freq_shift]")
{
    const double fs = 1e6;
    struct Case
    {
        double tone, shift;
    };
    for (Case c : {Case{100e3, -100e3}, Case{0, 250e3}, Case{-50e3, 20e3}, Case{300e3, 150e3}})
    {
        INFO("tone = " << c.tone << ", shift = " << c.shift);
        auto in = dsp_test::complexTone(50000, dsp::hz_to_rad(c.tone, fs), 0.7);
        auto input = std::make_shared<dsp::stream<complex_t>>();
        dsp::FreqShiftBlock blk(input, fs, c.shift);
        auto r = dsp_test::runBlock(blk, input, in, 4096);
        REQUIRE(r.completed);
        REQUIRE(r.output.size() == in.size());

        double expected = remainder(dsp::hz_to_rad(c.tone + c.shift, fs), 2 * M_PI);
        CHECK(dsp_test::estimateFreq(r.output, 0, r.output.size()) == Approx(expected).margin(1e-5));

        // Pure rotation, amplitude unchanged; starts at phase 0
        for (size_t i = 0; i < in.size(); i++)
            REQUIRE(r.output[i].norm() == Approx(0.7f).epsilon(1e-4));
        CHECK(dsp_test::absDiff(r.output[0], in[0]) < 1e-6f);
    }
}

TEST_CASE("FreqShiftBlock phase is continuous across buffers", "[common][dsp][freq_shift]")
{
    // DC in, so the output must be exactly the oscillator
    std::vector<complex_t> in(10000, complex_t(1, 0));
    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::FreqShiftBlock blk(input, 1.0, 0.01);
    auto r = dsp_test::runBlock(blk, input, in, 777);
    REQUIRE(r.completed);
    for (size_t i = 0; i < in.size(); i++)
    {
        double p = 2 * M_PI * 0.01 * i;
        REQUIRE(dsp_test::absDiff(r.output[i], complex_t(cos(p), sin(p))) < 1e-3f);
    }
}

TEST_CASE("FreqShiftBlock set_freq and set_freq_raw change the shift", "[common][dsp][freq_shift]")
{
    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::FreqShiftBlock blk(input, 1.0, 0.1);

    // Raw: radians per sample, before running
    blk.set_freq_raw(-0.25);
    auto r = dsp_test::runBlock(blk, input, std::vector<complex_t>(5000, complex_t(1, 0)), 1000);
    REQUIRE(r.completed);
    CHECK(dsp_test::estimateFreq(r.output, 0, r.output.size()) == Approx(-0.25).margin(1e-6));

    auto input2 = std::make_shared<dsp::stream<complex_t>>();
    dsp::FreqShiftBlock blk2(input2, 1.0, 0.1);
    blk2.set_freq(48000, 6000);
    auto r2 = dsp_test::runBlock(blk2, input2, std::vector<complex_t>(5000, complex_t(1, 0)), 1000);
    REQUIRE(r2.completed);
    CHECK(dsp_test::estimateFreq(r2.output, 0, r2.output.size()) == Approx(M_PI / 4).margin(1e-6));
}
