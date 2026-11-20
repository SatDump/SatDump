#include "common/dsp/filter/firdes.h"
#include "dsp/pll/costas.h"
#include "dsp/pll/costas_fast.h"
#include "dsp/pll/fll_band_edge.h"
#include "dsp/pll/pll_carrier_tracking.h"
#include "../filter/ndsp_sig_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;
using namespace satdump::ndsp;

namespace
{
    std::vector<complex_t> symbolsFor(int order, size_t n, uint32_t seed)
    {
        if (order == 2)
            return ndsp_sig_test::bpskSymbols(n, seed);
        if (order == 4)
            return ndsp_sig_test::qpskSymbols(n, seed);
        return ndsp_sig_test::psk8Symbols(n, seed);
    }

    // Runs a Costas loop over symbols with a frequency / phase offset,
    // checks that it locks onto the offset and that the symbols come out right
    template <typename B>
    void checkCostasLock(int order, double freq_offset, double phase_offset, double noise_sigma)
    {
        INFO("order " << order << ", offset " << freq_offset);
        const size_t n = 40000;
        auto syms = symbolsFor(order, n, 100 + order);
        auto in = ndsp_sig_test::rotate(syms, freq_offset, phase_offset);
        if (noise_sigma > 0)
            ndsp_sig_test::addNoise(in, noise_sigma, 5);

        auto costas = std::make_shared<B>();
        REQUIRE(ndsp_sig_test::setAll(*costas, {{"order", order}, {"loop_bw", 0.01}}) == Block::RES_OK);
        auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(costas, in);
        REQUIRE(r.completed);
        auto &out = r.output();
        REQUIRE(out.size() == n);

        // Locked onto the frequency offset
        CHECK(costas->get_cfg("freq").template get<double>() == Approx(freq_offset).margin(std::fabs(freq_offset) * 0.05 + (noise_sigma > 0 ? 6e-4 : 1e-4)));

        // Symbols recovered, up to the M-fold phase ambiguity, without any lag
        auto m = ndsp_sig_test::matchSymbols(syms, out, order, 20000, 0);
        CHECK(m.ratio > (noise_sigma > 0 ? 0.995 : 0.9999));

        // And the constellation is not rotated anymore
        double err = 0;
        for (size_t i = n - 20000; i < n; i++)
        {
            auto v = ndsp_sig_test::toStd(out[i]);
            auto s = ndsp_sig_test::toStd(syms[i]) * std::polar(1.0, -2.0 * ndsp_sig_test::kPi * m.rot / order);
            err += std::abs(v * std::conj(s) / std::abs(v * std::conj(s)) - 1.0);
        }
        CHECK(err / 20000.0 < (noise_sigma > 0 ? 0.15 : 0.02));
    }
} // namespace

TEST_CASE("ndsp CostasBlock locks onto BPSK, QPSK and 8PSK with a frequency offset", "[dsp][pll]")
{
    checkCostasLock<CostasBlock>(2, 0.004, 0.7, 0.0);
    checkCostasLock<CostasBlock>(2, -0.006, -2.0, 0.0);
    checkCostasLock<CostasBlock>(4, 0.003, 0.3, 0.0);
    checkCostasLock<CostasBlock>(8, 0.001, 0.1, 0.0);
}

TEST_CASE("ndsp CostasBlock locks onto noisy QPSK", "[dsp][pll]")
{
    // Es/N0 ~ 17 dB
    checkCostasLock<CostasBlock>(4, -0.002, 1.0, 0.1);
}

TEST_CASE("ndsp CostasFastBlock locks onto BPSK, QPSK and 8PSK with a frequency offset", "[dsp][pll]")
{
    checkCostasLock<CostasFastBlock>(2, 0.004, 0.7, 0.0);
    checkCostasLock<CostasFastBlock>(2, -0.006, -2.0, 0.0);
    checkCostasLock<CostasFastBlock>(4, 0.003, 0.3, 0.0);
    checkCostasLock<CostasFastBlock>(8, 0.001, 0.1, 0.0);
    checkCostasLock<CostasFastBlock>(4, -0.002, 1.0, 0.1);
}

TEST_CASE("ndsp CostasBlock and CostasFastBlock clamp the frequency to freq_limit", "[dsp][pll]")
{
    const double offset = 0.01, limit = 0.002;
    auto in = ndsp_sig_test::rotate(ndsp_sig_test::bpskSymbols(20000, 9), offset);
    for (int fast = 0; fast < 2; fast++)
    {
        INFO("fast " << fast);
        std::shared_ptr<Block> blk;
        if (fast)
            blk = std::make_shared<CostasFastBlock>();
        else
            blk = std::make_shared<CostasBlock>();
        REQUIRE(ndsp_sig_test::setAll(*blk, {{"order", 2}, {"loop_bw", 0.01}, {"freq_limit", limit}}) == Block::RES_OK);
        auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(blk, in);
        REQUIRE(r.completed);
        auto &out = r.output();

        // Loop gains, as designed from loop_bw with a damping of sqrt(2) / 2
        double bw = 0.01, damping = std::sqrt(2.0) / 2.0, denom = 1.0 + 2.0 * damping * bw + bw * bw;
        double alpha = 4 * damping * bw / denom, beta = 4 * bw * bw / denom;

        // The proportional path still tracks the offset, but the clamped integrator leaves
        // a static phase error with alpha * error = offset - limit, error = sin(2 theta) / 2
        std::vector<complex_t> sq(out.size());
        for (size_t i = 0; i < out.size(); i++)
            sq[i] = ndsp_sig_test::fromStd(ndsp_sig_test::toStd(out[i]) * ndsp_sig_test::toStd(out[i]));
        CHECK(std::fabs(ndsp_sig_test::estimateFreq(sq, 10000, out.size())) < 1e-4);

        if (!fast)
        {
            CHECK(std::fabs(blk->get_cfg("freq").get<double>()) <= limit + 1e-6);
            double theta = 0.5 * std::asin(2.0 * (offset - limit) / alpha);
            std::complex<double> acc = 0;
            for (size_t i = 10000; i < sq.size(); i++)
                acc += ndsp_sig_test::toStd(sq[i]);
            // BPSK, so the phase is only known modulo pi
            double measured = std::arg(acc) / 2.0;
            CHECK(std::fabs(std::remainder(std::fabs(measured) - theta, ndsp_sig_test::kPi)) < 0.02);
        }
        else
        {
            // CostasFastBlock only applies the limit every 65 samples, so its frequency
            // may overshoot in between, by at most 65 * beta (|error| <= 1)
            CHECK(std::fabs(blk->get_cfg("freq").get<double>()) <= limit + 65 * beta);
        }
    }
}

TEST_CASE("ndsp CostasFastBlock with an unsupported order outputs nothing", "[dsp][pll]")
{
    CostasFastBlock c;
    REQUIRE(c.set_cfg("order", 3) == Block::RES_OK);
    c.init();
    auto out = ndsp_sig_test::processDirect<complex_t>(c, ndsp_sig_test::bpskSymbols(100, 1), 100);
    CHECK(out.empty());
}

TEST_CASE("ndsp CostasBlock configuration round-trip", "[dsp][pll]")
{
    CostasBlock c;
    CostasFastBlock cf;
    CHECK(c.d_id == "costas_cc");
    CHECK(cf.d_id == "costas_fast_cc");
    for (Block *b : {(Block *)&c, (Block *)&cf})
    {
        auto def = ndsp_sig_test::getAll(*b);
        CHECK(def["order"] == 2);
        CHECK(def["loop_bw"].get<float>() == Approx(0.004));
        CHECK(def["freq_limit"].get<float>() == Approx(1.0));
        CHECK(def["freq"].get<float>() == 0);

        // freq is a read-only status
        CHECK(ndsp_sig_test::setAll(*b, {{"order", 4}, {"loop_bw", 0.02}, {"freq_limit", 0.5}, {"freq", 0.3}}) == Block::RES_OK);
        auto got = ndsp_sig_test::getAll(*b);
        CHECK(got["order"] == 4);
        CHECK(got["loop_bw"].get<float>() == Approx(0.02));
        CHECK(got["freq_limit"].get<float>() == Approx(0.5));
        CHECK(got["freq"].get<float>() == 0);
        CHECK_THROWS(b->set_cfg("x", 1));
        CHECK_THROWS(b->get_cfg("x"));
    }
}

TEST_CASE("ndsp FLLBandEdgeBlock removes a frequency offset from shaped BPSK", "[dsp][pll]")
{
    const int sps = 4;
    const double offset = 0.03;
    // RRC-shaped BPSK
    auto syms = ndsp_sig_test::bpskSymbols(25000, 11);
    std::vector<complex_t> up(syms.size() * sps, complex_t(0, 0));
    for (size_t i = 0; i < syms.size(); i++)
        up[i * sps] = syms[i];
    auto rrc = dsp::firdes::root_raised_cosine(sps, sps, 1.0, 0.35, 31 * sps + 1);
    auto shaped = ndsp_sig_test::filterSame(up, rrc);
    auto in = ndsp_sig_test::rotate(shaped, offset, 0.4);

    auto fll = std::make_shared<FLLBandEdgeBlock>();
    REQUIRE(ndsp_sig_test::setAll(*fll, {{"samps_per_sym", (double)sps}, {"rolloff", 0.35}, {"filter_size", 45}, {"loop_bw", 0.006}}) == Block::RES_OK);
    auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(fll, in);
    REQUIRE(r.completed);
    auto &out = r.output();
    REQUIRE(out.size() == in.size());

    // The NCO runs at minus the offset
    CHECK(fll->get_cfg("freq").get<double>() == Approx(-offset).margin(0.003));

    // Squaring removes the BPSK modulation, leaving twice the residual offset
    std::vector<complex_t> sq(out.size());
    for (size_t i = 0; i < out.size(); i++)
        sq[i] = ndsp_sig_test::fromStd(ndsp_sig_test::toStd(out[i]) * ndsp_sig_test::toStd(out[i]));
    CHECK(std::fabs(ndsp_sig_test::estimateFreq(sq, out.size() / 2, out.size()) / 2.0) < 0.003);
}

TEST_CASE("ndsp FLLBandEdgeBlock configuration round-trip", "[dsp][pll]")
{
    FLLBandEdgeBlock fll;
    CHECK(fll.d_id == "fll_band_edge_cc");
    nlohmann::json cfg = {{"samps_per_sym", 3.0}, {"rolloff", 0.25}, {"filter_size", 21}, {"loop_bw", 0.01}};
    CHECK(ndsp_sig_test::setAll(fll, cfg) == Block::RES_OK);
    auto got = ndsp_sig_test::getAll(fll);
    CHECK(got["samps_per_sym"].get<double>() == 3.0);
    CHECK(got["rolloff"].get<float>() == Approx(0.25));
    CHECK(got["filter_size"] == 21);
    CHECK(got["loop_bw"].get<float>() == Approx(0.01));
    CHECK(got.contains("freq"));
    CHECK(fll.set_cfg("freq", 1.0) == Block::RES_OK);
    CHECK_THROWS(fll.set_cfg("x", 1));
    CHECK_THROWS(fll.get_cfg("x"));
}

TEST_CASE("ndsp PLLCarrierTrackingBlock default configuration", "[dsp][pll]")
{
    // Only the getters are exercised : the loop state itself is never initialized
    // from the parameters (see the report), so running it is undefined behaviour
    PLLCarrierTrackingBlock pll;
    CHECK(pll.d_id == "pll_carrier_tracking_cc");
    CHECK(pll.get_cfg("loop_bw").get<float>() == Approx(0.01));
    CHECK(pll.get_cfg("freq_max").get<float>() == Approx(1));
    CHECK(pll.get_cfg("freq_min").get<float>() == Approx(-1));
    CHECK(pll.get_cfg_list().size() == 3);
    CHECK_THROWS(pll.get_cfg("x"));
}
