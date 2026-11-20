#include "common/dsp/clock_recovery/clock_recovery_gardner.h"
#include "common/dsp/clock_recovery/clock_test_signal.h"
#include "helpers/prefilled.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Approx;
using Catch::Matchers::WithinAbs;

namespace
{
    const float kGardnerMuGain = 0.05f;
    const float kGardnerOmegaGain = kGardnerMuGain * kGardnerMuGain / 4.0f;
    const float kGardnerOmegaLimit = 0.005f;
} // namespace

TEST_CASE("GardnerClockRecoveryBlock<float> outputs one sample per symbol on the eye opening", "[common][dsp][clock_recovery][gardner]")
{
    struct Case
    {
        double sps, true_sps, offset;
    };
    for (Case c : {Case{2, 2.0, 0.0}, Case{2, 2.0, 0.8}, Case{4, 4.01, 1.1}, Case{4, 3.99, 2.9}})
    {
        INFO("sps = " << c.sps << ", true sps = " << c.true_sps << ", offset = " << c.offset);
        auto sym = dsp_test_clock::randomSymbols(8000, 50);
        auto sig = dsp_test_clock::shapeSymbols(sym, c.true_sps, c.offset);

        auto input = std::make_shared<dsp::stream<float>>();
        // Built over zeroed memory, as the float version never initialises last_sample (see UNTESTED_ISSUES.md)
        auto gd = test_helpers::makePrefilled<dsp::GardnerClockRecoveryBlock<float>>(0, input, c.sps, kGardnerOmegaGain, 0.5f, kGardnerMuGain, kGardnerOmegaLimit);
        auto r = dsp_test::runBlock(*gd, input, sig, 4096);
        REQUIRE(r.completed);

        CHECK_THAT((double)r.output.size(), WithinAbs(sym.size(), 0.01 * sym.size()));

        size_t start = r.output.size() / 4;
        size_t open = 0;
        for (size_t k = start; k < r.output.size(); k++)
            open += fabs(r.output[k]) > 0.7f;
        CHECK((double)open / (r.output.size() - start) > 0.99);

        double match = dsp_test_clock::bestMatch(r.output.size(), start, sym, [&](size_t k) { return r.output[k] > 0; });
        CHECK(match > 0.999);
    }
}

TEST_CASE("GardnerClockRecoveryBlock<complex_t> recovers QPSK symbols", "[common][dsp][clock_recovery][gardner]")
{
    auto isym = dsp_test_clock::randomSymbols(8000, 51);
    auto qsym = dsp_test_clock::randomSymbols(8000, 52);
    auto i = dsp_test_clock::shapeSymbols(isym, 2.002, 0.7);
    auto q = dsp_test_clock::shapeSymbols(qsym, 2.002, 0.7);
    std::vector<complex_t> sig(i.size());
    for (size_t n = 0; n < sig.size(); n++)
        sig[n] = complex_t(i[n], q[n]);

    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::GardnerClockRecoveryBlock<complex_t> gd(input, 2.0f, kGardnerOmegaGain, 0.5f, kGardnerMuGain, kGardnerOmegaLimit);
    auto r = dsp_test::runBlock(gd, input, sig, 3000);
    REQUIRE(r.completed);
    CHECK_THAT((double)r.output.size(), WithinAbs(isym.size(), 0.01 * isym.size()));

    size_t start = r.output.size() / 4;
    size_t open = 0;
    for (size_t k = start; k < r.output.size(); k++)
        open += fabs(r.output[k].real) > 0.7f && fabs(r.output[k].imag) > 0.7f;
    CHECK((double)open / (r.output.size() - start) > 0.99);

    CHECK(dsp_test_clock::bestMatch(r.output.size(), start, isym, [&](size_t k) { return r.output[k].real > 0; }) > 0.999);
    CHECK(dsp_test_clock::bestMatch(r.output.size(), start, qsym, [&](size_t k) { return r.output[k].imag > 0; }) > 0.999);
}
