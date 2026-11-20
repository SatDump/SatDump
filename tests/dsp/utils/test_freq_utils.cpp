#include "../ndsp_test_helpers.h"
#include "dsp/utils/freq_shift.h"
#include "dsp/utils/hilbert.h"
#include "dsp/utils/quadrature_demod.h"
#include "dsp/utils/vco.h"
#include <catch2/catch_test_macros.hpp>
#include <complex>

using namespace satdump::ndsp;

///////////////////////////////////////////////////////////////////////////////
// Frequency shift
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp FreqShiftBlock shifts by freq_shift / samplerate", "[dsp][utils][freq_shift]")
{
    // 1 kHz at 8 kHz : the phase advances 2*pi/8 per sample
    auto b = std::make_shared<FreqShiftBlock>();
    REQUIRE(ndsp_test::configure(*b, {{"freq_shift", 1000.0}, {"samplerate", 8000.0}}) == Block::RES_OK);
    std::vector<complex_t> in(10000, complex_t(1, 0));
    auto r = ndsp_test::runSimple<complex_t, complex_t>(b, in, 1024);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == in.size());
    for (size_t n = 0; n < in.size(); n++)
    {
        double p = 2.0 * ndsp_test::kPi * (double)(n % 8) / 8.0;
        REQUIRE(ndsp_test::absDiff(r.out()[n], complex_t(cos(p), sin(p))) < 1e-3);
    }
}

TEST_CASE("ndsp FreqShiftBlock moves a tone and keeps its amplitude", "[dsp][utils][freq_shift]")
{
    const double fs = 2e6, shift = -150e3;
    auto in = ndsp_test::complexTone(20000, 0.3, 0.7);
    auto b = std::make_shared<FreqShiftBlock>();
    REQUIRE(ndsp_test::configure(*b, {{"freq_shift", shift}, {"samplerate", fs}}) == Block::RES_OK);
    auto r = ndsp_test::runSimple<complex_t, complex_t>(b, in, 4096);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == in.size());
    double expected = 0.3 + 2.0 * ndsp_test::kPi * shift / fs;
    CHECK(std::fabs(ndsp_test::estimateFreq(r.out(), 0, r.out().size()) - expected) < 1e-5);
    for (auto &v : r.out())
        REQUIRE(std::fabs(std::hypot(v.real, v.imag) - 0.7) < 1e-3);
}

TEST_CASE("ndsp FreqShiftBlock with zero shift is the identity", "[dsp][utils][freq_shift]")
{
    auto in = ndsp_test::randomComplex(3000, 21);
    auto b = std::make_shared<FreqShiftBlock>();
    auto r = ndsp_test::runSimple<complex_t, complex_t>(b, in, 1000);
    REQUIRE(r.completed);
    CHECK(ndsp_test::maxAbsDiff(r.out(), in) < 1e-6);
}

// Bug : freq_shift.h:50-53, init() in raw_shift mode only sets phase_delta and
// never initializes the rotator phase, which is default constructed to 0+0j
// (complex_t() is zero). So a fresh block in raw mode outputs zeros (then NaN
// after VOLK renormalizes 0/|0|) instead of a rotated signal.
TEST_CASE("ndsp FreqShiftBlock raw shift rotates by freq_shift radians per sample", "[dsp][utils][freq_shift][!shouldfail]")
{
    auto b = std::make_shared<FreqShiftBlock>();
    REQUIRE(ndsp_test::configure(*b, {{"raw_shift", true}, {"freq_shift", 0.1}}) == Block::RES_OK);
    std::vector<complex_t> in(256, complex_t(1, 0));
    auto r = ndsp_test::runSimple<complex_t, complex_t>(b, in, 256);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == in.size());
    bool all_ok = true;
    for (size_t n = 0; n < in.size(); n++)
        all_ok = all_ok && ndsp_test::absDiff(r.out()[n], complex_t(cos(0.1 * n), sin(0.1 * n))) < 1e-3;
    CHECK(all_ok);
}

TEST_CASE("ndsp FreqShiftBlock id and config round-trip", "[dsp][utils][freq_shift]")
{
    FreqShiftBlock b;
    CHECK(b.d_id == "freq_shift_cc");
    CHECK(b.get_cfg("freq_shift") == 0.0);
    CHECK(b.get_cfg("samplerate") == 6e6);
    CHECK(b.get_cfg("raw_shift") == false);
    Block &bb = b;
    REQUIRE(bb.set_cfg(nlohmann::json{{"freq_shift", 123.5}, {"samplerate", 1e6}, {"raw_shift", true}}) == Block::RES_OK);
    nlohmann::json all = bb.get_cfg();
    CHECK(all.size() == 3);
    CHECK(all["freq_shift"] == 123.5);
    CHECK(all["samplerate"] == 1e6);
    CHECK(all["raw_shift"] == true);
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}

///////////////////////////////////////////////////////////////////////////////
// Hilbert
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp HilbertBlock produces the analytic signal of a cosine", "[dsp][utils][hilbert]")
{
    // With 65 taps the output is delayed by 33 samples :
    // real = x[n - 33], imag = (h * x)[n - 1], whose own group delay is 32
    const int delay = 33;
    for (double w : {ndsp_test::kPi / 2, ndsp_test::kPi / 3, 2.0})
    {
        INFO("w = " << w);
        auto in = ndsp_test::realTone(6000, w);
        auto b = std::make_shared<HilbertBlock>();
        auto r = ndsp_test::runSimple<float, complex_t>(b, in, 1000);
        REQUIRE(r.completed);
        REQUIRE(r.out().size() == in.size());
        auto &out = r.out();

        double max_err_re = 0, max_err_im = 0;
        for (size_t n = 200; n < out.size(); n++)
        {
            max_err_re = std::max(max_err_re, std::fabs(out[n].real - cos(w * ((double)n - delay))));
            max_err_im = std::max(max_err_im, std::fabs(out[n].imag - sin(w * ((double)n - delay))));
        }
        CHECK(max_err_re < 1e-5);
        CHECK(max_err_im < 0.02);
        // Positive frequency only
        CHECK(std::fabs(ndsp_test::estimateFreq(out, 200, out.size()) - w) < 1e-3);
    }
}

TEST_CASE("ndsp HilbertBlock output does not depend on buffer boundaries", "[dsp][utils][hilbert]")
{
    auto in = ndsp_test::randomFloats(5000, 22);
    auto b1 = std::make_shared<HilbertBlock>();
    auto b2 = std::make_shared<HilbertBlock>();
    auto r1 = ndsp_test::runSimple<float, complex_t>(b1, in, 5000);
    auto r2 = ndsp_test::runSimple<float, complex_t>(b2, in, 77);
    REQUIRE(r1.completed);
    REQUIRE(r2.completed);
    CHECK(ndsp_test::maxAbsDiff(r1.out(), r2.out()) < 1e-6);
    // Zero initial history : the first 33 real outputs are 0
    for (int i = 0; i < 33; i++)
        REQUIRE(r1.out()[i].real == 0.0f);
    CHECK(r1.out()[33].real == in[0]);
}

TEST_CASE("ndsp HilbertBlock config round-trip and tap count", "[dsp][utils][hilbert]")
{
    HilbertBlock b;
    CHECK(b.d_id == "hilbert_fc");
    CHECK(b.get_inputs()[0].type == DSP_SAMPLE_TYPE_F32);
    CHECK(b.get_outputs()[0].type == DSP_SAMPLE_TYPE_CF32);
    CHECK(b.get_cfg("ntaps") == 65);
    CHECK(b.get_cfg("buffer_size") == 8192 * 8);
    b.set_cfg("ntaps", 31);
    b.set_cfg("buffer_size", 4096);
    CHECK(b.get_cfg("ntaps") == 31);
    CHECK(b.get_cfg("buffer_size") == 4096);
    CHECK(b.get_cfg_list().contains("ntaps"));
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}

TEST_CASE("ndsp HilbertBlock with fewer taps changes the delay", "[dsp][utils][hilbert]")
{
    auto b = std::make_shared<HilbertBlock>();
    b->set_cfg("ntaps", 31);
    auto in = ndsp_test::randomFloats(500, 23);
    auto r = ndsp_test::runSimple<float, complex_t>(b, in, 100);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == in.size());
    // real = x[n - (31 - 31 / 2)] = x[n - 16]
    for (size_t n = 16; n < in.size(); n++)
        REQUIRE(r.out()[n].real == in[n - 16]);
}

TEST_CASE("ndsp HilbertBlock rejects an even number of taps on start", "[dsp][utils][hilbert]")
{
    // firdes::hilbert only accepts odd tap counts. This is the first init() of
    // the block, so nothing was allocated for the taps yet and it is safe to destroy.
    HilbertBlock b;
    b.set_cfg("ntaps", 64);
    CHECK_THROWS_AS(b.start(), std::out_of_range);
}

///////////////////////////////////////////////////////////////////////////////
// Quadrature demod
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp QuadratureDemodBlock outputs the phase difference", "[dsp][utils][quadrature_demod]")
{
    for (double w : {0.05, -0.7, 2.9, -3.0})
    {
        INFO("w = " << w);
        auto in = ndsp_test::complexTone(2000, w);
        auto b = std::make_shared<QuadratureDemodBlock>();
        auto r = ndsp_test::runSimple<complex_t, float>(b, in, 300);
        REQUIRE(r.completed);
        REQUIRE(r.out().size() == in.size());
        // The phase history starts at 0, and the tone starts at phase 0
        CHECK(std::fabs(r.out()[0]) < 1e-6);
        for (size_t i = 1; i < in.size(); i++)
            REQUIRE(std::fabs(r.out()[i] - w) < 1e-3);
    }
}

TEST_CASE("ndsp QuadratureDemodBlock divides by the configured gain", "[dsp][utils][quadrature_demod]")
{
    // set_cfg stores 1 / gain, so output = phase_diff / gain
    auto b = std::make_shared<QuadratureDemodBlock>();
    b->set_cfg("gain", 4.0);
    CHECK(b->get_cfg("gain").get<double>() == 4.0);
    auto in = ndsp_test::complexTone(500, 0.8, 3.0);
    auto r = ndsp_test::runSimple<complex_t, float>(b, in, 128);
    REQUIRE(r.completed);
    for (size_t i = 1; i < in.size(); i++)
        REQUIRE(std::fabs(r.out()[i] - 0.2f) < 1e-3);
}

TEST_CASE("ndsp QuadratureDemodBlock demodulates FM", "[dsp][utils][quadrature_demod]")
{
    // Instantaneous frequency follows a slow sine
    const size_t n = 4000;
    std::vector<double> f(n);
    std::vector<complex_t> in(n);
    double phase = 0;
    for (size_t i = 0; i < n; i++)
    {
        f[i] = 0.5 * sin(2 * ndsp_test::kPi * i / 800.0);
        phase += f[i];
        in[i] = complex_t(cos(phase), sin(phase));
    }
    auto b = std::make_shared<QuadratureDemodBlock>();
    auto r = ndsp_test::runSimple<complex_t, float>(b, in, 1000);
    REQUIRE(r.completed);
    for (size_t i = 1; i < n; i++)
        REQUIRE(std::fabs(r.out()[i] - f[i]) < 1e-3);
}

TEST_CASE("ndsp QuadratureDemodBlock id and config", "[dsp][utils][quadrature_demod]")
{
    QuadratureDemodBlock b;
    CHECK(b.d_id == "quadrature_demod_cf");
    CHECK(b.get_cfg("gain").get<double>() == 1.0);
    CHECK(b.get_cfg_list()["gain"]["type"] == "float");
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}

///////////////////////////////////////////////////////////////////////////////
// VCO
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp VCOBlock integrates its input into phase", "[dsp][utils][vco]")
{
    auto b = std::make_shared<VCOBlock>();
    REQUIRE(ndsp_test::configure(*b, {{"k", 0.5}, {"amp", 2.0}}) == Block::RES_OK);
    std::vector<float> in(3000, 0.5f);
    auto r = ndsp_test::runSimple<float, complex_t>(b, in, 512);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == in.size());
    // out[n] = amp * exp(j * k * sum(in[0..n-1])), the first output being at phase 0
    for (size_t n = 0; n < in.size(); n++)
    {
        double p = 0.25 * (double)n;
        REQUIRE(ndsp_test::absDiff(r.out()[n], complex_t(2 * cos(p), 2 * sin(p))) < 2e-3);
    }
}

TEST_CASE("ndsp VCOBlock frequency follows the input sign", "[dsp][utils][vco]")
{
    auto b = std::make_shared<VCOBlock>();
    std::vector<float> in(2000);
    for (size_t i = 0; i < in.size(); i++)
        in[i] = i < 1000 ? 0.3f : -1.2f;
    auto r = ndsp_test::runSimple<float, complex_t>(b, in, 700);
    REQUIRE(r.completed);
    CHECK(std::fabs(ndsp_test::estimateFreq(r.out(), 1, 1000) - 0.3) < 1e-4);
    CHECK(std::fabs(ndsp_test::estimateFreq(r.out(), 1001, 2000) + 1.2) < 1e-4);
    for (auto &v : r.out())
        REQUIRE(std::fabs(std::hypot(v.real, v.imag) - 1.0) < 1e-5);
}

TEST_CASE("ndsp VCOBlock id and config round-trip", "[dsp][utils][vco]")
{
    VCOBlock b;
    CHECK(b.d_id == "vco_fc");
    CHECK(b.get_cfg("k") == 1.0);
    CHECK(b.get_cfg("amp") == 1.0);
    b.set_cfg("k", 0.125);
    b.set_cfg("amp", 3.5);
    nlohmann::json all = ((Block &)b).get_cfg();
    CHECK(all.size() == 2);
    CHECK(all["k"] == 0.125);
    CHECK(all["amp"] == 3.5);
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}

TEST_CASE("ndsp VCOBlockDeviation derives k from deviation and samplerate", "[dsp][utils][vco]")
{
    auto b = std::make_shared<VCOBlockDeviation>();
    CHECK(b->d_id == "vco_dev_fc");
    // Set in both orders, k = 2 * pi * deviation / samplerate
    REQUIRE(ndsp_test::configure(*b, {{"deviation", 4800.0}}) == Block::RES_OK);
    REQUIRE(ndsp_test::configure(*b, {{"samplerate", 48000.0}}) == Block::RES_OK);
    CHECK(b->get_cfg("deviation") == 4800.0);
    CHECK(b->get_cfg("samplerate") == 48000.0);
    CHECK(b->get_cfg("amp") == 1.0);
    CHECK_THROWS(b->get_cfg("k"));
    CHECK_THROWS(b->set_cfg("k", 1));

    std::vector<float> in(1000, 1.0f);
    auto r = ndsp_test::runSimple<float, complex_t>(b, in, 256);
    REQUIRE(r.completed);
    CHECK(std::fabs(ndsp_test::estimateFreq(r.out(), 0, r.out().size()) - 2 * ndsp_test::kPi * 0.1) < 1e-4);
}

// Bug : vco.h:64, VCOBlockDeviation reports samplerate = 48 kHz and
// deviation = 2 kHz by default but never derives k from them, so until one of
// them is set again the VCO runs with the base class k = 1 rad/sample per unit.
TEST_CASE("ndsp VCOBlockDeviation defaults match its reported config", "[dsp][utils][vco][!shouldfail]")
{
    auto b = std::make_shared<VCOBlockDeviation>();
    std::vector<float> in(1000, 1.0f);
    auto r = ndsp_test::runSimple<float, complex_t>(b, in, 256);
    REQUIRE(r.completed);
    double expected = 2 * ndsp_test::kPi * b->get_cfg("deviation").get<double>() / b->get_cfg("samplerate").get<double>();
    CHECK(std::fabs(ndsp_test::estimateFreq(r.out(), 0, r.out().size()) - expected) < 1e-4);
}
