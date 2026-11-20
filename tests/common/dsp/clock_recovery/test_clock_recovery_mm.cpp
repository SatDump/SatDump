#include "common/dsp/clock_recovery/clock_recovery_mm.h"
#include "clock_test_signal.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

namespace
{
    // Loop parameters in the range used by the demodulators, a bit faster to keep the test short
    const float kMuGain = 0.05f;
    const float kOmegaGain = kMuGain * kMuGain / 4.0f;
    const float kOmegaLimit = 0.005f;
}

TEST_CASE("MMClockRecoveryBlock<float> outputs one sample per symbol on the eye opening", "[common][dsp][clock_recovery][mm]")
{
    struct Case
    {
        double true_sps;
        double offset;
    };
    // Nominal 4 sps: exact rate, a timing offset, and a small rate mismatch within omega limit
    for (Case c : {Case{4.0, 0.0}, Case{4.0, 1.7}, Case{4.01, 0.6}, Case{3.99, 2.3}})
    {
        INFO("true sps = " << c.true_sps << ", offset = " << c.offset);
        auto sym = dsp_test_clock::randomSymbols(8000, 40);
        auto sig = dsp_test_clock::shapeSymbols(sym, c.true_sps, c.offset);

        auto input = std::make_shared<dsp::stream<float>>();
        dsp::MMClockRecoveryBlock<float> mm(input, 4.0f, kOmegaGain, 0.5f, kMuGain, kOmegaLimit);
        auto r = dsp_test::runBlock(mm, input, sig, 4096);
        REQUIRE(r.completed);

        // About one output per input symbol
        CHECK(fabs((double)r.output.size() - sym.size()) < 0.01 * sym.size());

        // After convergence, samples sit on the symbols (RC is ISI free at the right instant)
        size_t start = r.output.size() / 4;
        size_t open = 0;
        for (size_t k = start; k < r.output.size(); k++)
            open += fabs(r.output[k]) > 0.7f;
        CHECK((double)open / (r.output.size() - start) > 0.99);

        double match = dsp_test_clock::bestMatch(r.output.size(), start, sym, [&](size_t k)
                                                 { return r.output[k] > 0; });
        CHECK(match > 0.999);
    }
}

TEST_CASE("MMClockRecoveryBlock<float> works at 2 samples per symbol", "[common][dsp][clock_recovery][mm]")
{
    auto sym = dsp_test_clock::randomSymbols(8000, 41);
    auto sig = dsp_test_clock::shapeSymbols(sym, 2.0, 0.5);

    auto input = std::make_shared<dsp::stream<float>>();
    dsp::MMClockRecoveryBlock<float> mm(input, 2.0f, kOmegaGain, 0.5f, kMuGain, kOmegaLimit);
    auto r = dsp_test::runBlock(mm, input, sig, 1000);
    REQUIRE(r.completed);
    CHECK(fabs((double)r.output.size() - sym.size()) < 0.01 * sym.size());

    size_t start = r.output.size() / 4;
    double match = dsp_test_clock::bestMatch(r.output.size(), start, sym, [&](size_t k)
                                             { return r.output[k] > 0; });
    CHECK(match > 0.999);
}

TEST_CASE("MMClockRecoveryBlock<complex_t> recovers QPSK symbols", "[common][dsp][clock_recovery][mm]")
{
    auto isym = dsp_test_clock::randomSymbols(8000, 42);
    auto qsym = dsp_test_clock::randomSymbols(8000, 43);
    auto i = dsp_test_clock::shapeSymbols(isym, 4.005, 1.3);
    auto q = dsp_test_clock::shapeSymbols(qsym, 4.005, 1.3);
    std::vector<complex_t> sig(i.size());
    for (size_t n = 0; n < sig.size(); n++)
        sig[n] = complex_t(i[n], q[n]);

    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::MMClockRecoveryBlock<complex_t> mm(input, 4.0f, kOmegaGain, 0.5f, kMuGain, kOmegaLimit);
    auto r = dsp_test::runBlock(mm, input, sig, 4096);
    REQUIRE(r.completed);
    CHECK(fabs((double)r.output.size() - isym.size()) < 0.01 * isym.size());

    size_t start = r.output.size() / 4;
    size_t open = 0;
    for (size_t k = start; k < r.output.size(); k++)
        open += fabs(r.output[k].real) > 0.7f && fabs(r.output[k].imag) > 0.7f;
    CHECK((double)open / (r.output.size() - start) > 0.99);

    CHECK(dsp_test_clock::bestMatch(r.output.size(), start, isym, [&](size_t k)
                                    { return r.output[k].real > 0; }) > 0.999);
    CHECK(dsp_test_clock::bestMatch(r.output.size(), start, qsym, [&](size_t k)
                                    { return r.output[k].imag > 0; }) > 0.999);
}
