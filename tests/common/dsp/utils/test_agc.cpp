#include "common/dsp/utils/agc.h"
#include "../dsp_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

TEST_CASE("AGCBlock<complex_t> converges to the reference magnitude", "[common][dsp][agc]")
{
    for (double amp : {0.05, 0.5, 20.0})
    {
        INFO("input amplitude = " << amp);
        // Gain time constant is 1 / (rate * amp) samples, 2000 at worst here
        auto in = dsp_test::complexTone(40000, 0.1, amp);
        auto input = std::make_shared<dsp::stream<complex_t>>();
        dsp::AGCBlock<complex_t> agc(input, 1e-2, 1.0f, 1.0f, 65536);
        auto r = dsp_test::runBlock(agc, input, in, 4096);
        REQUIRE(r.completed);
        REQUIRE(r.output.size() == in.size());

        for (size_t i = 30000; i < in.size(); i++)
            REQUIRE(r.output[i].norm() == Approx(1.0f).margin(1e-3));
        // Gain only, no phase change
        CHECK(dsp_test::estimateFreq(r.output, 30000, in.size()) == Approx(0.1).epsilon(1e-4));
    }
}

TEST_CASE("AGCBlock<float> converges to the reference magnitude", "[common][dsp][agc]")
{
    // Constant input magnitude, alternating sign
    std::vector<float> in(20000);
    for (size_t i = 0; i < in.size(); i++)
        in[i] = (i % 2) ? 0.2f : -0.2f;

    auto input = std::make_shared<dsp::stream<float>>();
    dsp::AGCBlock<float> agc(input, 1e-2, 0.5f, 1.0f, 0);
    auto r = dsp_test::runBlock(agc, input, in, 4096);
    REQUIRE(r.completed);
    for (size_t i = 10000; i < in.size(); i++)
    {
        REQUIRE(fabs(r.output[i]) == Approx(0.5f).margin(1e-4));
        REQUIRE((r.output[i] > 0) == (in[i] > 0));
    }
}

TEST_CASE("AGCBlock limits gain to max_gain", "[common][dsp][agc]")
{
    auto in = dsp_test::complexTone(20000, 0.0, 1e-3);
    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::AGCBlock<complex_t> agc(input, 1e-1, 1.0f, 1.0f, 50.0f);
    auto r = dsp_test::runBlock(agc, input, in, 4096);
    REQUIRE(r.completed);
    // Would need a gain of 1000, stuck at 50
    for (size_t i = 10000; i < in.size(); i++)
        REQUIRE(r.output[i].norm() == Approx(0.05f).epsilon(1e-3));
}

TEST_CASE("AGCBlock first sample uses the initial gain", "[common][dsp][agc]")
{
    std::vector<complex_t> in(10, complex_t(0.3f, 0.4f));
    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::AGCBlock<complex_t> agc(input, 1e-3, 1.0f, 2.0f, 0);
    auto r = dsp_test::runBlock(agc, input, in, 10);
    REQUIRE(r.completed);
    CHECK(r.output[0].real == Approx(0.6f));
    CHECK(r.output[0].imag == Approx(0.8f));
    // gain += rate * (reference - |out|) = 2 + 1e-3 * (1 - 1) = 2
    CHECK(r.output[1].norm() == Approx(1.0f));
}
