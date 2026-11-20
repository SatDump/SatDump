#include "dsp/resampling/rational_resampler.h"
#include "../filter/ndsp_sig_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;
using namespace satdump::ndsp;

namespace
{
    template <typename T>
    std::shared_ptr<RationalResamplerBlock<T>> makeResampler(double interp, double decim)
    {
        auto r = std::make_shared<RationalResamplerBlock<T>>();
        REQUIRE(ndsp_sig_test::setAll(*r, {{"interpolation", interp}, {"decimation", decim}}) == Block::RES_OK);
        return r;
    }
} // namespace

TEST_CASE("ndsp RationalResamplerBlock<complex_t> output rate, tone frequency and gain", "[dsp][resampling]")
{
    struct Case
    {
        double interp, decim;
    };
    // Including ratios given as sample rates, reduced by their GCD internally
    for (auto c : {Case{1, 2}, Case{3, 2}, Case{2, 1}, Case{2, 3}, Case{48000, 96000}, Case{48000, 250000}, Case{5, 1}})
    {
        double ratio = c.interp / c.decim;
        INFO("interpolation " << c.interp << " decimation " << c.decim);

        // Tone at 0.05 cycles per output sample, well inside the passband
        const size_t n = 60000;
        double w_out = 2 * ndsp_sig_test::kPi * 0.05;
        double w_in = w_out * ratio;
        auto in = ndsp_sig_test::complexTone(n, w_in, 0.8);

        auto res = makeResampler<complex_t>(c.interp, c.decim);
        auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(res, in, ndsp_sig_test::chunked(4096));
        REQUIRE(r.completed);
        auto &out = r.output();

        // Output count, minus the filter history held back
        double expected = n * ratio;
        CHECK(out.size() <= expected + 1);
        CHECK(out.size() >= expected - 400 * std::max(1.0, ratio));

        size_t skip = out.size() / 10;
        CHECK(ndsp_sig_test::estimateFreq(out, skip, out.size()) == Approx(w_out).epsilon(1e-3));
        CHECK(ndsp_sig_test::toneAmplitude(out, w_out, skip, out.size()) == Approx(0.8).epsilon(0.01));
    }
}

TEST_CASE("ndsp RationalResamplerBlock<float> decimation keeps a DC level and rejects aliases", "[dsp][resampling]")
{
    // DC + a tone that would alias after decimation by 4
    const size_t n = 80000;
    auto in = ndsp_sig_test::realTone(n, 2 * ndsp_sig_test::kPi * 0.3, 1.0);
    for (auto &v : in)
        v += 0.5f;
    auto res = makeResampler<float>(1, 4);
    auto r = ndsp_sig_test::runBlock<float, float>(res, in);
    REQUIRE(r.completed);
    auto &out = r.output();
    REQUIRE(out.size() == Approx(n / 4).margin(100));

    // 0.3 cycles/sample aliases to 0.2 cycles/output sample
    size_t skip = 500;
    double mean = 0;
    for (size_t i = skip; i < out.size(); i++)
        mean += out[i];
    mean /= (double)(out.size() - skip);
    CHECK(mean == Approx(0.5).epsilon(0.01));
    CHECK(ndsp_sig_test::toDb(ndsp_sig_test::toneAmplitude(out, 2 * ndsp_sig_test::kPi * 0.2, skip, out.size())) < -40);
}

TEST_CASE("ndsp RationalResamplerBlock with custom single tap passes samples through", "[dsp][resampling]")
{
    auto res = std::make_shared<RationalResamplerBlock<float>>();
    REQUIRE(ndsp_sig_test::setAll(*res, {{"interpolation", 1}, {"decimation", 1}, {"taps", std::vector<float>{1.0f}}}) == Block::RES_OK);
    std::vector<float> in(10000);
    for (size_t i = 0; i < in.size(); i++)
        in[i] = (float)i;
    auto r = ndsp_sig_test::runBlock<float, float>(res, in, ndsp_sig_test::chunked(333));
    REQUIRE(r.completed);
    auto &out = r.output();
    // One sample is always held back, waiting for the next buffer
    REQUIRE(out.size() == in.size() - 1);
    for (size_t i = 0; i < out.size(); i++)
        REQUIRE(out[i] == in[i]);
}

TEST_CASE("ndsp RationalResamplerBlock custom taps implement an interpolator", "[dsp][resampling]")
{
    // Interpolation by 2 with taps {1, 1} : zero-order hold
    auto res = std::make_shared<RationalResamplerBlock<float>>();
    REQUIRE(ndsp_sig_test::setAll(*res, {{"interpolation", 2}, {"decimation", 1}, {"taps", std::vector<float>{1.0f, 1.0f}}}) == Block::RES_OK);
    std::vector<float> in = {1, 2, 3, 4, 5, 6};
    auto r = ndsp_sig_test::runBlock<float, float>(res, in);
    REQUIRE(r.completed);
    std::vector<float> expected = {1, 1, 2, 2, 3, 3, 4, 4, 5, 5};
    CHECK(r.output() == expected);
}

TEST_CASE("ndsp RationalResamplerBlock configuration round-trip", "[dsp][resampling]")
{
    RationalResamplerBlock<complex_t> r;
    CHECK(r.d_id == "rational_resampler_cc");
    CHECK(RationalResamplerBlock<float>().d_id == "rational_resampler_ff");
    auto def = ndsp_sig_test::getAll(r);
    CHECK(def == nlohmann::json({{"interpolation", 1.0}, {"decimation", 2.0}}));

    CHECK(ndsp_sig_test::setAll(r, {{"interpolation", 3}, {"decimation", 7}}) == Block::RES_OK);
    CHECK(ndsp_sig_test::getAll(r) == nlohmann::json({{"interpolation", 3.0}, {"decimation", 7.0}}));
    std::vector<float> t = {0.5f, 0.25f};
    CHECK(r.set_cfg("taps", t) == Block::RES_OK);
    CHECK(r.get_cfg("taps").get<std::vector<float>>() == t);
    CHECK_THROWS(r.set_cfg("x", 1));
    CHECK_THROWS(r.get_cfg("x"));
}
