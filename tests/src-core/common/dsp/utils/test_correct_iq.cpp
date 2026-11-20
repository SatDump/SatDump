#include "common/dsp/dsp_test_helpers.h"
#include "common/dsp/utils/correct_iq.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

namespace
{
    std::vector<complex_t> toneWithDc(size_t n, complex_t dc)
    {
        auto v = dsp_test::complexTone(n, 0.3, 0.5);
        for (auto &x : v)
            x += dc;
        return v;
    }

    double meanAbsDc(const std::vector<complex_t> &x, size_t start)
    {
        // A tone at 0.3 rad/sample averages out over a long window, leaving the DC
        std::complex<double> acc = 0;
        for (size_t i = start; i < x.size(); i++)
            acc += std::complex<double>(x[i].real, x[i].imag);
        return std::abs(acc / (double)(x.size() - start));
    }
} // namespace

TEST_CASE("CorrectIQBlock<complex_t> removes a DC offset with the default rate", "[common][dsp][correct_iq]")
{
    // Default alpha = 1e-4: time constant of 10k samples
    auto in = toneWithDc(200000, complex_t(0.3f, -0.2f));
    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::CorrectIQBlock<complex_t> blk(input);
    auto r = dsp_test::runBlock(blk, input, in, 8192);
    REQUIRE(r.completed);
    REQUIRE(r.output.size() == in.size());

    CHECK(meanAbsDc(in, 150000) == Approx(0.36).margin(0.01));
    CHECK(meanAbsDc(r.output, 150000) < 0.005);
    // The tone itself goes through
    CHECK(dsp_test::rms(r.output, 150000, in.size()) == Approx(0.5).margin(0.01));
}

TEST_CASE("CorrectIQBlock<float> removes a DC offset with the default rate", "[common][dsp][correct_iq]")
{
    std::vector<float> in(200000);
    for (size_t i = 0; i < in.size(); i++)
        in[i] = 1.5f + ((i % 2) ? 0.5f : -0.5f);
    auto input = std::make_shared<dsp::stream<float>>();
    dsp::CorrectIQBlock<float> blk(input);
    auto r = dsp_test::runBlock(blk, input, in, 8192);
    REQUIRE(r.completed);
    size_t mismatches = 0, first = 0;
    for (size_t i = 150000; i < in.size(); i++)
        if (!(fabs(r.output[i]) == Approx(0.5f).margin(0.01)) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);
}

// Bug: correct_iq.cpp:6-9 - the constructor parameter `alpha` only sets beta = 1 - alpha;
// the member `alpha` stays at its 1e-4 default. With any other rate, the moving average
// converges to (1e-4 / alpha) * DC instead of DC, so the offset is barely removed.
TEST_CASE("CorrectIQBlock<complex_t> removes a DC offset with a custom rate", "[common][dsp][correct_iq][!shouldfail]")
{
    // alpha = 1e-2: time constant of 100 samples, should be fully converged after 20k
    auto in = toneWithDc(20000, complex_t(0.3f, -0.2f));
    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::CorrectIQBlock<complex_t> blk(input, 1e-2f);
    auto r = dsp_test::runBlock(blk, input, in, 4096);
    REQUIRE(r.completed);
    CHECK(meanAbsDc(r.output, 10000) < 0.01);
}
