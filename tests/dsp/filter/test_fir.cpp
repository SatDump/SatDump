#include "common/dsp/filter/firdes.h"
#include "dsp/filter/decimating_fir.h"
#include "dsp/filter/fir.h"
#include "dsp/filter/gaussian.h"
#include "dsp/filter/hpf.h"
#include "dsp/filter/lpf.h"
#include "dsp/filter/rrc.h"
#include "ndsp_sig_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;
using namespace satdump::ndsp;

namespace
{
    std::vector<float> randomTaps(size_t n, uint32_t seed)
    {
        ndsp_sig_test::Lcg rng(seed);
        std::vector<float> t(n);
        for (auto &v : t)
            v = rng.symmetric();
        return t;
    }

    std::vector<complex_t> randomComplex(size_t n, uint32_t seed)
    {
        ndsp_sig_test::Lcg rng(seed);
        std::vector<complex_t> t(n);
        for (auto &v : t)
            v = complex_t(rng.symmetric(), rng.symmetric());
        return t;
    }

    template <typename T>
    void checkMatches(const std::vector<T> &out, const std::vector<std::complex<double>> &ref, double tol)
    {
        for (size_t i = 0; i < out.size(); i++)
        {
            INFO("sample " << i);
            if constexpr (std::is_same_v<T, complex_t>)
            {
                REQUIRE(out[i].real == Approx(ref[i].real()).margin(tol));
                REQUIRE(out[i].imag == Approx(ref[i].imag()).margin(tol));
            }
            else
            {
                REQUIRE(out[i] == Approx(ref[i].real()).margin(tol));
            }
        }
    }
} // namespace

TEST_CASE("ndsp FIRBlock<float> matches a direct convolution across buffer boundaries", "[dsp][filter]")
{
    for (size_t ntaps : {1, 7, 31, 64})
    {
        INFO("ntaps = " << ntaps);
        auto taps = randomTaps(ntaps, 10 + ntaps);
        auto in = randomTaps(20000, 99);

        auto fir = std::make_shared<FIRBlock<float>>();
        REQUIRE(fir->set_cfg("taps", taps) == Block::RES_OK);
        ndsp_sig_test::RunOptions opt;
        opt.chunk_cycle = {1, 100, 4096, 37, 1000, 2};
        auto r = ndsp_sig_test::runBlock<float, float>(fir, in, opt);
        REQUIRE(r.completed);

        // The first output is the first full overlap, so ntaps fewer outputs than inputs
        auto ref = ndsp_sig_test::firReference(in, taps);
        REQUIRE(r.output().size() == in.size() - ntaps);
        checkMatches(r.output(), ref, 1e-4);
    }
}

TEST_CASE("ndsp FIRBlock<complex_t, float> matches a direct convolution", "[dsp][filter]")
{
    auto taps = randomTaps(33, 3);
    auto in = randomComplex(30000, 4);
    auto fir = std::make_shared<FIRBlock<complex_t, float>>();
    REQUIRE(fir->set_cfg("taps", taps) == Block::RES_OK);
    auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(fir, in, ndsp_sig_test::chunked(1500));
    REQUIRE(r.completed);
    auto ref = ndsp_sig_test::firReference(in, taps);
    REQUIRE(r.output().size() == ref.size());
    checkMatches(r.output(), ref, 1e-4);
}

TEST_CASE("ndsp FIRBlock<complex_t, complex_t> matches a direct convolution", "[dsp][filter]")
{
    auto taps = randomComplex(21, 5);
    auto in = randomComplex(30000, 6);
    auto fir = std::make_shared<FIRBlock<complex_t, complex_t>>();
    REQUIRE(fir->set_cfg("taps", taps) == Block::RES_OK);
    auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(fir, in, ndsp_sig_test::chunked(999));
    REQUIRE(r.completed);
    auto ref = ndsp_sig_test::firReference(in, taps);
    REQUIRE(r.output().size() == ref.size());
    checkMatches(r.output(), ref, 1e-4);
}

TEST_CASE("ndsp FIRBlock taps configuration round-trip and IDs", "[dsp][filter]")
{
    FIRBlock<float> ff;
    FIRBlock<complex_t> cf;
    FIRBlock<complex_t, complex_t> cc;
    CHECK(ff.d_id == "fir_ff");
    CHECK(cf.d_id == "fir_cc");
    CHECK(cc.d_id == "firc_cc");
    CHECK(LPF_Block<FIRBlock<complex_t>>().d_id == "lpffir_cc");

    std::vector<float> t = {0.25f, -1.5f, 3.0f};
    CHECK(ff.set_cfg("taps", t) == Block::RES_OK);
    CHECK(ff.get_cfg("taps").get<std::vector<float>>() == t);
    CHECK(ff.needs_reinit);

    std::vector<complex_t> ct = {complex_t(1, 2), complex_t(-3, 0.5)};
    CHECK(cc.set_cfg("taps", ct) == Block::RES_OK);
    auto back = cc.get_cfg("taps");
    REQUIRE(back.size() == 2);
    CHECK(back[0]["real"] == 1.0);
    CHECK(back[0]["imag"] == 2.0);
    CHECK(back[1]["real"] == -3.0);
    CHECK(back[1]["imag"] == 0.5);

    CHECK_THROWS(ff.set_cfg("nope", 1));
    CHECK_THROWS(ff.get_cfg("nope"));
}

TEST_CASE("ndsp FIRBlock taps can be changed between runs", "[dsp][filter]")
{
    auto fir = std::make_shared<FIRBlock<float>>();
    std::vector<float> in = {1, 2, 3, 4, 5, 6, 7, 8};

    REQUIRE(fir->set_cfg("taps", std::vector<float>{1.0f}) == Block::RES_OK);
    auto r1 = ndsp_sig_test::runBlock<float, float>(fir, in);
    REQUIRE(r1.completed);
    CHECK(r1.output() == std::vector<float>{2, 3, 4, 5, 6, 7, 8});

    // y[i] = x[i+2] - x[i+1]
    REQUIRE(fir->set_cfg("taps", std::vector<float>{1.0f, -1.0f}) == Block::RES_OK);
    auto r2 = ndsp_sig_test::runBlock<float, float>(fir, in);
    REQUIRE(r2.completed);
    CHECK(r2.output() == std::vector<float>{1, 1, 1, 1, 1, 1});
}

TEST_CASE("ndsp LPF_Block designs a low-pass filter and filters tones", "[dsp][filter]")
{
    const double fs = 1e6, cutoff = 100e3, tw = 20e3;
    auto lpf = std::make_shared<LPF_Block<FIRBlock<complex_t>>>();
    REQUIRE(ndsp_sig_test::setAll(*lpf, {{"samplerate", fs}, {"cutoff", cutoff}, {"transition_width", tw}, {"gain", 2.0}}) == Block::RES_OK);

    auto taps = lpf->get_cfg("taps").get<std::vector<float>>();
    CHECK(taps == ::dsp::firdes::low_pass(2.0, fs, cutoff, tw));
    REQUIRE(taps.size() % 2 == 1);
    for (size_t i = 0; i < taps.size() / 2; i++)
        REQUIRE(taps[i] == Approx(taps[taps.size() - 1 - i]).margin(1e-7));
    CHECK(ndsp_sig_test::magResponse(taps, 0.0) == Approx(2.0).epsilon(1e-3));

    // In-band tone keeps its amplitude (x2), out of band tone is gone
    const size_t n = 40000;
    double w_in = 2 * ndsp_sig_test::kPi * 30e3 / fs, w_out = 2 * ndsp_sig_test::kPi * 250e3 / fs;
    auto in = ndsp_sig_test::complexTone(n, w_in, 0.5);
    auto noise = ndsp_sig_test::complexTone(n, w_out, 0.5);
    for (size_t i = 0; i < n; i++)
        in[i] = in[i] + noise[i];
    auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(lpf, in);
    REQUIRE(r.completed);
    auto &out = r.output();
    REQUIRE(out.size() == n - taps.size());
    CHECK(ndsp_sig_test::toneAmplitude(out, w_in, 1000, out.size()) == Approx(1.0).epsilon(0.01));
    CHECK(ndsp_sig_test::toDb(ndsp_sig_test::toneAmplitude(out, w_out, 1000, out.size())) < -40);

    auto cfg = ndsp_sig_test::getAll(*lpf);
    CHECK(cfg == nlohmann::json({{"gain", 2.0}, {"samplerate", fs}, {"cutoff", cutoff}, {"transition_width", tw}}));
}

TEST_CASE("ndsp HPF_Block rejects DC and passes high frequencies", "[dsp][filter]")
{
    const double fs = 48000;
    auto hpf = std::make_shared<HPF_Block<FIRBlock<float>>>();
    REQUIRE(ndsp_sig_test::setAll(*hpf, {{"samplerate", fs}, {"cutoff", 6000}, {"transition_width", 2000}}) == Block::RES_OK);
    auto taps = hpf->get_cfg("taps").get<std::vector<float>>();
    CHECK(ndsp_sig_test::magResponse(taps, 0.0) < 1e-2);
    CHECK(ndsp_sig_test::magResponse(taps, ndsp_sig_test::kPi) == Approx(1.0).epsilon(0.01));

    const size_t n = 30000;
    double w_hi = 2 * ndsp_sig_test::kPi * 15000 / fs;
    auto in = ndsp_sig_test::realTone(n, w_hi, 0.7);
    for (auto &v : in)
        v += 3.0f; // DC
    auto r = ndsp_sig_test::runBlock<float, float>(hpf, in);
    REQUIRE(r.completed);
    auto &out = r.output();
    double mean = 0;
    for (size_t i = 1000; i < out.size(); i++)
        mean += out[i];
    mean /= (double)(out.size() - 1000);
    CHECK(std::fabs(mean) < 0.03);
    CHECK(ndsp_sig_test::toneAmplitude(out, w_hi, 1000, out.size()) == Approx(0.7).epsilon(0.02));
}

TEST_CASE("ndsp RRC_Block taps form a Nyquist pulse when matched", "[dsp][filter]")
{
    // 4 samples per symbol
    auto rrc = std::make_shared<RRC_Block<FIRBlock<complex_t>>>();
    REQUIRE(ndsp_sig_test::setAll(*rrc, {{"samplerate", 4e6}, {"symbolrate", 1e6}, {"alpha", 0.35}, {"ntaps", 101}, {"gain", 1.0}}) == Block::RES_OK);
    auto taps = rrc->get_cfg("taps").get<std::vector<float>>();
    REQUIRE(taps.size() == 101);
    CHECK(taps == ::dsp::firdes::root_raised_cosine(1.0, 4e6, 1e6, 0.35, 101));
    for (size_t i = 0; i < taps.size() / 2; i++)
        REQUIRE(taps[i] == Approx(taps[taps.size() - 1 - i]).margin(1e-6));

    // RRC * RRC is a raised cosine : zero at every non-zero multiple of the symbol period
    std::vector<double> t(taps.begin(), taps.end());
    auto rc = ndsp_sig_test::convolveFull(t, t);
    size_t c = rc.size() / 2;
    for (int k = 1; k <= 8; k++)
    {
        INFO("symbol offset " << k);
        CHECK(std::fabs(rc[c + 4 * k] / rc[c]) < 0.02);
        CHECK(std::fabs(rc[c - 4 * k] / rc[c]) < 0.02);
    }

    auto cfg = ndsp_sig_test::getAll(*rrc);
    CHECK(cfg["ntaps"] == 101);
    CHECK(cfg["alpha"].get<double>() == Approx(0.35));
}

TEST_CASE("ndsp Gaussian_Block designs normalized gaussian taps", "[dsp][filter]")
{
    auto g = std::make_shared<Gaussian_Block<FIRBlock<float>>>();
    REQUIRE(ndsp_sig_test::setAll(*g, {{"gain", 3.0}, {"symbolrate", 4.0}, {"alpha", 0.5}, {"ntaps", 25}}) == Block::RES_OK);
    auto taps = g->get_cfg("taps").get<std::vector<float>>();
    REQUIRE(taps.size() == 25);
    CHECK(taps == ::dsp::firdes::gaussian(3.0, 4.0, 0.5, 25));
    double sum = 0;
    for (float v : taps)
    {
        REQUIRE(v > 0);
        sum += v;
    }
    CHECK(sum == Approx(3.0).epsilon(1e-5));

    // A DC input comes out with the filter gain
    std::vector<float> in(1000, 1.0f);
    auto r = ndsp_sig_test::runBlock<float, float>(g, in);
    REQUIRE(r.completed);
    REQUIRE(r.output().size() == 1000 - 25);
    for (float v : r.output())
        REQUIRE(v == Approx(3.0f).epsilon(1e-4));

    CHECK(ndsp_sig_test::getAll(*g) == nlohmann::json({{"gain", 3.0}, {"symbolrate", 4.0}, {"alpha", 0.5}, {"ntaps", 25}}));
}

// Bug : decimating_fir.cpp:100 (and :120, :138) resets decim_pos to 1 right before
// incrementing it, so the next output comes after decimation - 1 inputs instead of
// decimation. Decimation 2 does not decimate at all, and decimation 1 (the default)
// outputs a single sample and then nothing, ever.
TEST_CASE("ndsp DecimatingFIRBlock with decimation 1 behaves like FIRBlock", "[dsp][filter][!shouldfail]")
{
    auto taps = randomTaps(15, 20);
    auto in = randomTaps(10000, 21);
    auto dec = std::make_shared<DecimatingFIRBlock<float>>();
    REQUIRE(dec->set_cfg("taps", taps) == Block::RES_OK);
    REQUIRE(dec->set_cfg("decimation", 1) == Block::RES_OK);
    auto r = ndsp_sig_test::runBlock<float, float>(dec, in);
    REQUIRE(r.completed);
    auto ref = ndsp_sig_test::firReference(in, taps);
    REQUIRE(r.output().size() == ref.size());
    checkMatches(r.output(), ref, 1e-4);
}

TEST_CASE("ndsp DecimatingFIRBlock decimates by the configured factor", "[dsp][filter][!shouldfail]")
{
    auto taps = randomTaps(15, 22);
    auto in = randomTaps(20000, 23);
    for (int d : {2, 4, 5})
    {
        INFO("decimation " << d);
        auto dec = std::make_shared<DecimatingFIRBlock<complex_t>>();
        REQUIRE(dec->set_cfg("taps", taps) == Block::RES_OK);
        REQUIRE(dec->set_cfg("decimation", d) == Block::RES_OK);
        auto r = ndsp_sig_test::runBlock<complex_t, complex_t>(dec, ndsp_sig_test::toComplex(in), ndsp_sig_test::chunked(1234));
        REQUIRE(r.completed);
        auto ref = ndsp_sig_test::firReference(in, taps);
        CHECK(std::abs((double)r.output().size() - (double)ref.size() / d) <= 1.0);
    }
}

TEST_CASE("ndsp DecimatingFIRBlock outputs are samples of the full-rate filter output", "[dsp][filter]")
{
    // Whatever the output rate, each output must be one of the FIR outputs, in order
    auto taps = randomTaps(9, 24);
    auto in = randomTaps(5000, 25);
    auto dec = std::make_shared<DecimatingFIRBlock<float>>();
    REQUIRE(dec->set_cfg("taps", taps) == Block::RES_OK);
    REQUIRE(dec->set_cfg("decimation", 4) == Block::RES_OK);
    auto r = ndsp_sig_test::runBlock<float, float>(dec, in, ndsp_sig_test::chunked(333));
    REQUIRE(r.completed);
    auto ref = ndsp_sig_test::firReference(in, taps);
    REQUIRE(r.output().size() > 0);
    size_t j = 0;
    for (float v : r.output())
    {
        while (j < ref.size() && std::fabs(ref[j].real() - v) > 1e-4)
            j++;
        REQUIRE(j < ref.size());
        j++;
    }
}

TEST_CASE("ndsp DecimatingFIRBlock configuration round-trip", "[dsp][filter]")
{
    DecimatingFIRBlock<complex_t> d;
    CHECK(d.d_id == "decimating_fir_cc");
    CHECK(DecimatingFIRBlock<complex_t, complex_t>().d_id == "decimating_firc_cc");
    CHECK(DecimatingFIRBlock<float>().d_id == "decimating_fir_ff");
    CHECK(d.get_cfg("decimation") == 1);
    CHECK(d.set_cfg("decimation", 8) == Block::RES_OK);
    CHECK(d.get_cfg("decimation") == 8);
    // Clamped to 1
    CHECK(d.set_cfg("decimation", -3) == Block::RES_OK);
    CHECK(d.get_cfg("decimation") == 1);
    std::vector<float> t = {1, 2, 3};
    CHECK(d.set_cfg("taps", t) == Block::RES_OK);
    CHECK(d.get_cfg("taps").get<std::vector<float>>() == t);
    CHECK_THROWS(d.set_cfg("x", 0));
    CHECK_THROWS(d.get_cfg("x"));
}
