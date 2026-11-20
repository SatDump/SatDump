#include "common/dsp/pll/bpsk_carrier_pll.h"
#include "../dsp_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

TEST_CASE("BPSKCarrierPLLBlock output goes to zero on an unmodulated carrier", "[common][dsp][bpsk_carrier_pll]")
{
    for (double f : {0.002, -0.004})
    {
        INFO("freq = " << f);
        auto in = dsp_test::complexTone(30000, f, 1.0, 2.0);
        auto input = std::make_shared<dsp::stream<complex_t>>();
        // Typical loop: beta = alpha^2 / 4
        dsp::BPSKCarrierPLLBlock pll(input, 0.01f, 0.01f * 0.01f / 4, 0.1f);
        auto r = dsp_test::runBlock(pll, input, in, 4096);
        REQUIRE(r.completed);
        REQUIRE(r.output.size() == in.size());

        // Output is the quadrature component of the carrier-removed signal: 0 when locked
        for (size_t i = in.size() / 2; i < in.size(); i++)
            REQUIRE(fabs(r.output[i]) < 0.01f);
    }
}

TEST_CASE("BPSKCarrierPLLBlock demodulates residual-carrier PM", "[common][dsp][bpsk_carrier_pll]")
{
    // Phase modulation by +-1 rad around a carrier with a frequency offset
    const size_t n = 60000;
    const double mod = 1.0;
    dsp_test::Lcg rng(80);
    std::vector<complex_t> in(n);
    std::vector<float> data(n);
    float d = 1;
    for (size_t i = 0; i < n; i++)
    {
        if (i % 8 == 0)
            d = rng.sign();
        data[i] = d;
        double p = 0.0005 * i + mod * d + 0.3;
        in[i] = complex_t(cos(p), sin(p));
    }

    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::BPSKCarrierPLLBlock pll(input, 0.002f, 0.002f * 0.002f / 4, 0.1f);
    auto r = dsp_test::runBlock(pll, input, in, 4096);
    REQUIRE(r.completed);

    // Once locked on the carrier, out = sin(mod * d)
    size_t ok = 0, amp_ok = 0;
    for (size_t i = n / 2; i < n; i++)
    {
        ok += (r.output[i] > 0) == (data[i] > 0);
        amp_ok += fabs(fabs(r.output[i]) - sin(mod)) < 0.15;
    }
    CHECK((double)ok / (n / 2) > 0.99);
    CHECK((double)amp_ok / (n / 2) > 0.95);
}
