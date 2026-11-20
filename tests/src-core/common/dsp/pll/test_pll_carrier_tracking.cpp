#include "common/dsp/dsp_test_helpers.h"
#include "common/dsp/pll/pll_carrier_tracking.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

TEST_CASE("PLLCarrierTrackingBlock locks onto a carrier and brings it to DC", "[common][dsp][pll_carrier_tracking]")
{
    for (double f : {0.02, -0.03, 0.001})
    {
        INFO("freq = " << f);
        auto in = dsp_test::complexTone(20000, f, 0.8, 1.0);

        auto input = std::make_shared<dsp::stream<complex_t>>();
        dsp::PLLCarrierTrackingBlock pll(input, 0.01f, 0.1f, -0.1f);
        auto r = dsp_test::runBlock(pll, input, in, 4096);
        REQUIRE(r.completed);
        REQUIRE(r.output.size() == in.size());

        CHECK(pll.getFreq() == Approx(f).margin(1e-4));

        // Locked: output is a constant at the carrier amplitude, on the real axis.
        // The VCO uses fast_cos / fast_sin, so it is only unit-magnitude to ~0.1 %
        size_t mismatches = 0, first = 0;
        for (size_t i = in.size() / 2; i < in.size(); i++)
            if ((!(fabs(atan2(r.output[i].imag, r.output[i].real)) < 0.01) || !(r.output[i].norm() == Approx(0.8f).epsilon(5e-3))) && mismatches++ == 0)
                first = i;
        INFO("first mismatch at sample " << first);
        REQUIRE(mismatches == 0);
    }
}

TEST_CASE("PLLCarrierTrackingBlock clamps the frequency between min and max", "[common][dsp][pll_carrier_tracking]")
{
    auto in = dsp_test::complexTone(20000, 0.05);
    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::PLLCarrierTrackingBlock pll(input, 0.02f, 0.01f, -0.01f);
    REQUIRE(dsp_test::runBlock(pll, input, in, 4096).completed);
    CHECK(pll.getFreq() == Approx(0.01f));

    auto in2 = dsp_test::complexTone(20000, -0.05);
    auto input2 = std::make_shared<dsp::stream<complex_t>>();
    dsp::PLLCarrierTrackingBlock pll2(input2, 0.02f, 0.01f, -0.01f);
    REQUIRE(dsp_test::runBlock(pll2, input2, in2, 4096).completed);
    CHECK(pll2.getFreq() == Approx(-0.01f));
}

TEST_CASE("PLLCarrierTrackingBlock keeps slow modulation on the carrier", "[common][dsp][pll_carrier_tracking]")
{
    // A slow loop only tracks the carrier: fast (+-0.5 rad, every 4 samples) phase modulation remains visible
    const size_t n = 60000;
    dsp_test::Lcg rng(70);
    std::vector<complex_t> in(n);
    std::vector<float> data(n);
    float d = 1;
    for (size_t i = 0; i < n; i++)
    {
        if (i % 4 == 0)
            d = rng.sign();
        data[i] = d;
        double p = 0.004 * i + 0.5 * d;
        in[i] = complex_t(cos(p), sin(p));
    }

    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::PLLCarrierTrackingBlock pll(input, 0.001f, 0.1f, -0.1f);
    auto r = dsp_test::runBlock(pll, input, in, 4096);
    REQUIRE(r.completed);
    CHECK(pll.getFreq() == Approx(0.004).margin(2e-4));

    size_t ok = 0;
    for (size_t i = n / 2; i < n; i++)
        ok += (r.output[i].imag > 0) == (data[i] > 0);
    CHECK((double)ok / (n / 2) > 0.99);
}
