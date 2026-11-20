#include "dsp/clock_recovery/clock_recovery_gardner.h"
#include "dsp/clock_recovery/clock_recovery_mm.h"
#include "dsp/clock_recovery/clock_recovery_mm_fast.h"
#include "dsp/clock_recovery/simple_gardner_recovery.h"
#include "dsp/clock_recovery/simple_zc_recovery.h"
#include "../filter/ndsp_sig_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;
using namespace satdump::ndsp;

namespace
{
    const size_t kSyms = 12000;

    // Raised-cosine shaped BPSK (real) or QPSK (complex), i.e. what comes out of a matched filter,
    // with a fractional timing offset
    std::vector<float> realSignal(double sps, uint32_t seed, std::vector<complex_t> &syms)
    {
        syms = ndsp_sig_test::bpskSymbols(kSyms, seed);
        std::vector<float> s(kSyms);
        for (size_t i = 0; i < kSyms; i++)
            s[i] = syms[i].real;
        return ndsp_sig_test::rcShapeReal(s, sps, 0.37, 0.5);
    }

    std::vector<complex_t> complexSignal(double sps, uint32_t seed, std::vector<complex_t> &syms)
    {
        syms = ndsp_sig_test::qpskSymbols(kSyms, seed);
        return ndsp_sig_test::rcShape(syms, sps, 0.37, 0.5);
    }

    // About one output per symbol, and the symbols are recovered after convergence
    template <typename T>
    void checkRecovered(const std::vector<T> &out, const std::vector<complex_t> &syms, double nin, double sps, int order)
    {
        double expected = nin / sps;
        CHECK((double)out.size() == Approx(expected).epsilon(0.01));
        std::vector<complex_t> c;
        if constexpr (std::is_same_v<T, float>)
            c = ndsp_sig_test::toComplex(out);
        else
            c = out;
        auto m = ndsp_sig_test::matchSymbols(syms, c, order, kSyms / 2, 30);
        INFO("lag " << m.lag << " rot " << m.rot);
        CHECK(m.ratio > 0.999);

        // Sampled close to the symbol peaks, so the eye is open : magnitudes close to 1
        double minmag = 1e9, avgmag = 0;
        for (size_t i = c.size() - kSyms / 2; i < c.size(); i++)
        {
            double m = std::abs(ndsp_sig_test::toStd(c[i]));
            minmag = std::min(minmag, m);
            avgmag += m;
        }
        avgmag /= (double)(kSyms / 2);
        CHECK(minmag > 0.5);
        CHECK(avgmag == Approx(1.0).margin(0.1));
    }

    template <typename B>
    std::shared_ptr<B> makeRecovery(double sps)
    {
        auto b = std::make_shared<B>();
        REQUIRE(b->set_cfg("omega", sps) == Block::RES_OK);
        return b;
    }
} // namespace

TEST_CASE("ndsp MMClockRecoveryBlock<float> recovers BPSK symbols", "[dsp][clock_recovery]")
{
    for (double sps : {4.0, 4.3, 7.0})
    {
        INFO("sps " << sps);
        std::vector<complex_t> syms;
        auto in = realSignal(sps, 1, syms);
        auto r = ndsp_sig_test::runBlock<float, float>(makeRecovery<MMClockRecoveryBlock<float>>(sps), in);
        REQUIRE(r.completed);
        checkRecovered(r.output(), syms, in.size(), sps, 2);
    }
}

TEST_CASE("ndsp MMClockRecoveryBlock<complex_t> recovers QPSK symbols", "[dsp][clock_recovery]")
{
    for (double sps : {2.5, 4.0, 5.2})
    {
        INFO("sps " << sps);
        std::vector<complex_t> syms;
        auto in = complexSignal(sps, 2, syms);
        auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(makeRecovery<MMClockRecoveryBlock<complex_t>>(sps), in);
        REQUIRE(r.completed);
        checkRecovered(r.output(), syms, in.size(), sps, 4);
    }
}

TEST_CASE("ndsp MMClockRecoveryFastBlock recovers BPSK and QPSK symbols", "[dsp][clock_recovery]")
{
    for (double sps : {4.0, 6.6})
    {
        INFO("sps " << sps);
        std::vector<complex_t> syms;
        auto in = realSignal(sps, 3, syms);
        auto r = ndsp_sig_test::runBlock<float, float>(makeRecovery<MMClockRecoveryFastBlock<float>>(sps), in);
        REQUIRE(r.completed);
        checkRecovered(r.output(), syms, in.size(), sps, 2);

        auto cin = complexSignal(sps, 4, syms);
        auto rc = ndsp_sig_test::runBlock<complex_t, complex_t>(makeRecovery<MMClockRecoveryFastBlock<complex_t>>(sps), cin);
        REQUIRE(rc.completed);
        checkRecovered(rc.output(), syms, cin.size(), sps, 4);
    }
}

TEST_CASE("ndsp GardnerClockRecoveryBlock recovers BPSK and QPSK symbols", "[dsp][clock_recovery]")
{
    for (double sps : {4.0, 5.5})
    {
        INFO("sps " << sps);
        std::vector<complex_t> syms;
        auto in = realSignal(sps, 5, syms);
        auto r = ndsp_sig_test::runBlock<float, float>(makeRecovery<GardnerClockRecoveryBlock<float>>(sps), in);
        REQUIRE(r.completed);
        checkRecovered(r.output(), syms, in.size(), sps, 2);

        auto cin = complexSignal(sps, 6, syms);
        auto rc = ndsp_sig_test::runBlock<complex_t, complex_t>(makeRecovery<GardnerClockRecoveryBlock<complex_t>>(sps), cin);
        REQUIRE(rc.completed);
        checkRecovered(rc.output(), syms, cin.size(), sps, 4);
    }
}

TEST_CASE("ndsp MMClockRecoveryBlock tracks a small symbol rate error", "[dsp][clock_recovery]")
{
    // Actual rate 0.2 % off the configured omega, inside the default 0.5 % omega limit
    std::vector<complex_t> syms;
    auto in = complexSignal(4.0 * 1.002, 7, syms);
    auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(makeRecovery<MMClockRecoveryBlock<complex_t>>(4.0), in);
    REQUIRE(r.completed);
    checkRecovered(r.output(), syms, in.size(), 4.0 * 1.002, 4);
}

TEST_CASE("ndsp SimpleGardnerRecoveryBlock recovers QPSK symbols", "[dsp][clock_recovery]")
{
    for (double sps : {4.0, 3.3, 8.0})
    {
        INFO("sps " << sps);
        std::vector<complex_t> syms;
        auto in = complexSignal(sps, 8, syms);
        auto blk = std::make_shared<SimpleGardnerRecoveryBlock>();
        REQUIRE(blk->set_cfg("sps", sps) == Block::RES_OK);
        auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(blk, in);
        REQUIRE(r.completed);
        checkRecovered(r.output(), syms, in.size(), sps, 4);
    }
}

TEST_CASE("ndsp SimpleZeroCrossingRecoveryBlock recovers BPSK symbols", "[dsp][clock_recovery]")
{
    for (double sps : {5.0, 4.0, 9.7})
    {
        INFO("sps " << sps);
        std::vector<complex_t> syms;
        auto in = realSignal(sps, 9, syms);
        auto blk = std::make_shared<SimpleZeroCrossingRecoveryBlock>();
        REQUIRE(blk->set_cfg("sps", sps) == Block::RES_OK);
        auto r = ndsp_sig_test::runBlock<float, float>(blk, in);
        REQUIRE(r.completed);
        checkRecovered(r.output(), syms, in.size(), sps, 2);
    }
}

TEST_CASE("ndsp MMClockRecoveryBlock, GardnerClockRecoveryBlock and simple recovery blocks configuration round-trip", "[dsp][clock_recovery]")
{
    MMClockRecoveryBlock<complex_t> mmc;
    MMClockRecoveryBlock<float> mmf;
    MMClockRecoveryFastBlock<complex_t> mmfc;
    MMClockRecoveryFastBlock<float> mmff;
    GardnerClockRecoveryBlock<complex_t> gc;
    GardnerClockRecoveryBlock<float> gf;
    CHECK(mmc.d_id == "clock_recovery_mm_cc");
    CHECK(mmf.d_id == "clock_recovery_mm_ff");
    CHECK(mmfc.d_id == "fast_clock_recovery_mm_cc");
    CHECK(mmff.d_id == "fast_clock_recovery_mm_ff");
    CHECK(gc.d_id == "clock_recovery_gardner_cc");
    CHECK(gf.d_id == "clock_recovery_gardner_ff");

    nlohmann::json cfg = {{"omega", 3.5}, {"omegaGain", 0.001}, {"mu", 0.25}, {"muGain", 0.02}, {"omegaLimit", 0.01}, {"nfilt", 64}, {"ntaps", 16}};
    for (Block *b : {(Block *)&mmc, (Block *)&mmf, (Block *)&gc, (Block *)&gf})
    {
        INFO(b->d_id);
        auto def = ndsp_sig_test::getAll(*b);
        CHECK(def["omega"].get<float>() == 0);
        CHECK(def["mu"].get<float>() == Approx(0.5));
        CHECK(def["muGain"].get<float>() == Approx(8.7e-3));
        CHECK(def["omegaGain"].get<float>() == Approx(8.7e-3 * 8.7e-3 / 4.0));
        CHECK(def["omegaLimit"].get<float>() == Approx(0.005));
        CHECK(def["nfilt"] == 128);
        CHECK(def["ntaps"] == 8);

        CHECK(ndsp_sig_test::setAll(*b, cfg) == Block::RES_OK);
        auto got = ndsp_sig_test::getAll(*b);
        for (auto &it : cfg.items())
            CHECK(got[it.key()].get<double>() == Approx(it.value().get<double>()));
        CHECK_THROWS(b->set_cfg("x", 1));
        CHECK_THROWS(b->get_cfg("x"));
    }

    // The fast variant has no interpolator settings
    for (Block *b : {(Block *)&mmfc, (Block *)&mmff})
    {
        auto list = b->get_cfg_list();
        CHECK(list.size() == 5);
        CHECK(!list.contains("nfilt"));
        nlohmann::json c5 = {{"omega", 3.5}, {"omegaGain", 0.001}, {"mu", 0.25}, {"muGain", 0.02}, {"omegaLimit", 0.01}};
        CHECK(ndsp_sig_test::setAll(*b, c5) == Block::RES_OK);
        auto got = ndsp_sig_test::getAll(*b);
        for (auto &it : c5.items())
            CHECK(got[it.key()].get<double>() == Approx(it.value().get<double>()));
        CHECK_THROWS(b->set_cfg("nfilt", 1));
    }

    SimpleGardnerRecoveryBlock sg;
    SimpleZeroCrossingRecoveryBlock zc;
    CHECK(sg.d_id == "simple_gardner_recovery_c");
    CHECK(zc.d_id == "simple_zc_recovery_f");
    CHECK(ndsp_sig_test::getAll(sg) == nlohmann::json({{"sps", 4.0}, {"alpha", 0.02f}}));
    CHECK(ndsp_sig_test::getAll(zc) == nlohmann::json({{"sps", 5.0}, {"alpha", 0.1f}}));
    for (Block *b : {(Block *)&sg, (Block *)&zc})
    {
        CHECK(ndsp_sig_test::setAll(*b, {{"sps", 2.5}, {"alpha", 0.5}}) == Block::RES_OK);
        CHECK(ndsp_sig_test::getAll(*b) == nlohmann::json({{"sps", 2.5}, {"alpha", 0.5}}));
        CHECK_THROWS(b->set_cfg("x", 1));
        CHECK_THROWS(b->get_cfg("x"));
    }
}
