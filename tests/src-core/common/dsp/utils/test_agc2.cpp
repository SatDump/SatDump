#include "common/dsp/dsp_test_helpers.h"
#include "common/dsp/utils/agc2.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

TEST_CASE("AGC2Block<float> converges to the target mean magnitude", "[common][dsp][agc2]")
{
    for (double amp : {0.05, 3.0})
    {
        INFO("amplitude = " << amp);
        // Square wave of +-amp: |x| is constant
        std::vector<float> in(40000);
        for (size_t i = 0; i < in.size(); i++)
            in[i] = ((i / 3) % 2) ? amp : -amp;

        auto input = std::make_shared<dsp::stream<float>>();
        dsp::AGC2Block<float> agc(input, 1.0f, 1e-4f, 1e-2f);
        auto r = dsp_test::runBlock(agc, input, in, 4096);
        REQUIRE(r.completed);
        REQUIRE(r.output.size() == in.size());

        double mean_abs = 0;
        for (size_t i = 30000; i < in.size(); i++)
            mean_abs += fabs(r.output[i]);
        mean_abs /= 10000;
        CHECK(mean_abs == Approx(1.0).margin(0.02));
        size_t mismatches = 0, first = 0;
        for (size_t i = 30000; i < in.size(); i++)
            if (!((r.output[i] > 0) == (in[i] > 0)) && mismatches++ == 0)
                first = i;
        INFO("first mismatch at sample " << first);
        REQUIRE(mismatches == 0);
    }
}

// Bug: agc2.cpp:31-32 - the bias estimate is updated with the already bias-corrected sample
// (sample -= bias; bias = bias * (1 - d_bias) + sample * d_bias), so it converges to DC / 2
// instead of DC, and half of the DC offset is left in the output.
TEST_CASE("AGC2Block<float> removes a DC bias", "[common][dsp][agc2][!shouldfail]")
{
    // +-1 square wave sitting on a DC offset of 2
    std::vector<float> in(100000);
    for (size_t i = 0; i < in.size(); i++)
        in[i] = 2.0f + (((i / 5) % 2) ? 1.0f : -1.0f);

    auto input = std::make_shared<dsp::stream<float>>();
    dsp::AGC2Block<float> agc(input, 1.0f, 1e-3f, 1e-2f);
    auto r = dsp_test::runBlock(agc, input, in, 4096);
    REQUIRE(r.completed);

    double mean = 0;
    for (size_t i = 80000; i < in.size(); i++)
        mean += r.output[i];
    mean /= 20000;
    CHECK(mean == Approx(0.0).margin(0.05));
    size_t mismatches = 0, first = 0;
    for (size_t i = 80000; i < in.size(); i++)
        if (!(fabs(r.output[i]) == Approx(1.0f).margin(0.1)) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}

TEST_CASE("AGC2Block<float> passes exact zeros through untouched", "[common][dsp][agc2]")
{
    std::vector<float> in(1000, 0.0f);
    in[500] = 0.25f;
    auto input = std::make_shared<dsp::stream<float>>();
    dsp::AGC2Block<float> agc(input, 1.0f, 1e-3f, 1e-2f);
    auto r = dsp_test::runBlock(agc, input, in, 256);
    REQUIRE(r.completed);
    size_t mismatches = 0, first = 0;
    for (size_t i = 0; i < in.size(); i++)
        if (i != 500 && !(r.output[i] == 0.0f) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
    // moving_avg starts at the target, so the first non-zero sample gets gain 1
    CHECK(r.output[500] == Approx(0.25f));
}

TEST_CASE("AGC2Block<complex_t> is currently a passthrough", "[common][dsp][agc2]")
{
    // The complex branch of AGC2Block::work() is empty in the source
    auto in = dsp_test::complexTone(5000, 0.2, 7.0);
    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::AGC2Block<complex_t> agc(input, 1.0f, 1e-3f, 1e-2f);
    auto r = dsp_test::runBlock(agc, input, in, 1024);
    REQUIRE(r.completed);
    REQUIRE(r.output.size() == in.size());
    size_t mismatches = 0, first = 0;
    for (size_t i = 0; i < in.size(); i++)
        if (!(dsp_test::absDiff(r.output[i], in[i]) == 0.0f) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}
