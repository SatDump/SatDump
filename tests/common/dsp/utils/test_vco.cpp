#include "common/dsp/utils/vco.h"
#include "../dsp_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

TEST_CASE("VCOBlock constant input gives a tone at k * input", "[common][dsp][vco]")
{
    struct Case
    {
        float in, k, amp;
    };
    for (Case c : {Case{1.0f, 0.1f, 1.0f}, Case{-2.0f, 0.3f, 0.5f}, Case{0.5f, M_PI / 4, 2.0f}})
    {
        INFO("in = " << c.in << ", k = " << c.k << ", amp = " << c.amp);
        auto input = std::make_shared<dsp::stream<float>>();
        dsp::VCOBlock vco(input, c.k, c.amp);
        auto r = dsp_test::runBlock(vco, input, std::vector<float>(20000, c.in), 3000);
        REQUIRE(r.completed);
        REQUIRE(r.output.size() == 20000);

        // Starts at phase 0
        CHECK(r.output[0].real == Approx(c.amp));
        CHECK(r.output[0].imag == Approx(0.0f).margin(1e-6));
        CHECK(dsp_test::estimateFreq(r.output, 0, r.output.size()) == Approx(c.in * c.k).epsilon(1e-4));
        for (const auto &v : r.output)
            REQUIRE(hypotf(v.real, v.imag) == Approx(c.amp).epsilon(1e-5));
    }
}

TEST_CASE("VCOBlock output phase is the running sum of the input", "[common][dsp][vco]")
{
    dsp_test::Lcg rng(110);
    std::vector<float> in(10000);
    for (auto &v : in)
        v = rng.uniform() * 2 - 1;

    auto input = std::make_shared<dsp::stream<float>>();
    dsp::VCOBlock vco(input, 0.5f, 1.0f);
    auto r = dsp_test::runBlock(vco, input, in, 999);
    REQUIRE(r.completed);

    // out[n] = exp(j k sum_{i<n} in[i])
    double phase = 0;
    for (size_t n = 0; n < in.size(); n++)
    {
        REQUIRE(dsp_test::absDiff(r.output[n], complex_t(cos(phase), sin(phase))) < 1e-3f);
        phase += 0.5 * in[n];
    }
}
