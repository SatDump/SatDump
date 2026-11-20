#include "../ndsp_test_helpers.h"
#include "dsp/utils/blanker.h"
#include "dsp/utils/correct_iq.h"
#include "dsp/utils/cyclostationary_analysis.h"
#include "dsp/utils/delay_one_imag.h"
#include "dsp/utils/exponentiate.h"
#include "dsp/utils/repeat.h"
#include <catch2/catch_test_macros.hpp>
#include <complex>

using namespace satdump::ndsp;

///////////////////////////////////////////////////////////////////////////////
// Blanker
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp BlankerBlock passes samples through when not blanking", "[dsp][utils][blanker]")
{
    auto in = ndsp_test::randomFloats(2500, 11);
    auto b = std::make_shared<BlankerBlock<float>>();
    auto r = ndsp_test::runSimple<float, float>(b, in, 1000);
    REQUIRE(r.completed);
    CHECK(r.out() == in);
    CHECK(r.outputs[0].sizes == std::vector<uint32_t>{1000, 1000, 500});
}

TEST_CASE("ndsp BlankerBlock outputs zeros when blanking", "[dsp][utils][blanker]")
{
    auto in = ndsp_test::randomComplex(1500, 12);
    auto b = std::make_shared<BlankerBlock<complex_t>>();
    REQUIRE(ndsp_test::configure(*b, {{"blank", true}}) == Block::RES_OK);
    auto r = ndsp_test::runSimple<complex_t, complex_t>(b, in, 512);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == in.size());
    for (auto &v : r.out())
    {
        REQUIRE(v.real == 0.0f);
        REQUIRE(v.imag == 0.0f);
    }
}

TEST_CASE("ndsp BlankerBlock ids and config round-trip", "[dsp][utils][blanker]")
{
    BlankerBlock<float> f;
    BlankerBlock<complex_t> c;
    CHECK(f.d_id == "blanker_ff");
    CHECK(c.d_id == "blanker_cc");
    CHECK(f.get_cfg("blank") == false);
    f.set_cfg("blank", true);
    CHECK(f.get_cfg("blank") == true);
    CHECK(f.get_cfg_list()["blank"]["type"] == "bool");
    CHECK_THROWS(f.get_cfg("x"));
    CHECK_THROWS(f.set_cfg("x", 1));
}

///////////////////////////////////////////////////////////////////////////////
// CorrectIQ
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp CorrectIQBlock follows the single-pole DC estimator", "[dsp][utils][correct_iq]")
{
    // acc[n] = acc[n-1] * (1 - a) + x[n] * a, out = x - acc, a = 1e-4
    // For a constant x = d : out[n] = d * (1 - a)^(n+1)
    const double a = 0.0001;
    std::vector<complex_t> in(20000, complex_t(1.0f, -0.5f));
    auto b = std::make_shared<CorrectIQBlock<complex_t>>();
    auto r = ndsp_test::runSimple<complex_t, complex_t>(b, in, 4096);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == in.size());
    for (size_t n : {0, 1, 100, 5000, 19999})
    {
        double k = pow(1.0 - a, (double)n + 1);
        INFO("n = " << n);
        CHECK(std::fabs(r.out()[n].real - 1.0 * k) < 2e-4);
        CHECK(std::fabs(r.out()[n].imag + 0.5 * k) < 2e-4);
    }
}

TEST_CASE("ndsp CorrectIQBlock removes a DC offset and keeps the signal", "[dsp][utils][correct_iq]")
{
    // tone + DC, long enough for the estimator to settle ((1 - 1e-4)^100000 ~ 4.5e-5)
    const size_t n = 120000;
    auto tone = ndsp_test::realTone(n, 0.3, 0.5);
    std::vector<float> in(n);
    for (size_t i = 0; i < n; i++)
        in[i] = tone[i] + 2.0f;
    auto b = std::make_shared<CorrectIQBlock<float>>();
    auto r = ndsp_test::runSimple<float, float>(b, in, 8192);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == n);

    double mean = 0;
    for (size_t i = n - 10000; i < n; i++)
        mean += r.out()[i];
    mean /= 10000;
    CHECK(std::fabs(mean) < 0.01);
    // The tone itself goes through (alpha is tiny, so the error is mostly the residual DC)
    for (size_t i = n - 100; i < n; i++)
        REQUIRE(std::fabs(r.out()[i] - tone[i]) < 0.01);
}

TEST_CASE("ndsp CorrectIQBlock ids and config", "[dsp][utils][correct_iq]")
{
    CorrectIQBlock<complex_t> c;
    CorrectIQBlock<float> f;
    CHECK(c.d_id == "correct_iq_cc");
    CHECK(f.d_id == "correct_iq_ff");
    CHECK(c.get_inputs()[0].type == DSP_SAMPLE_TYPE_CF32);
    CHECK(f.get_outputs()[0].type == DSP_SAMPLE_TYPE_F32);
    CHECK(c.get_cfg_list().empty());
    CHECK_THROWS(c.get_cfg("alpha"));
    CHECK_THROWS(c.set_cfg("alpha", 0.1));
}

///////////////////////////////////////////////////////////////////////////////
// Cyclostationary analysis
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp CyclostationaryAnalysis outputs |conj(x) * x|", "[dsp][utils][cyclostationary_analysis]")
{
    auto in = ndsp_test::randomComplex(3000, 13, 2.0f);
    auto b = std::make_shared<CyclostationaryAnalysis>();
    auto r = ndsp_test::runSimple<complex_t, float>(b, in, 1000);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == in.size());
    for (size_t i = 0; i < in.size(); i++)
    {
        float expected = in[i].real * in[i].real + in[i].imag * in[i].imag;
        REQUIRE(std::fabs(r.out()[i] - expected) <= 1e-5f * std::max(1.0f, expected));
    }
    // Output buffers are allocated twice as large as needed, in floats
    CHECK(r.outputs[0].max_sizes == std::vector<uint32_t>{2000, 2000, 2000});
    CHECK(r.outputs[0].propagating_terminators == 1);
}

TEST_CASE("ndsp CyclostationaryAnalysis id and config", "[dsp][utils][cyclostationary_analysis]")
{
    CyclostationaryAnalysis b;
    CHECK(b.d_id == "cyclostationary_analysis_cf");
    CHECK(b.get_inputs()[0].type == DSP_SAMPLE_TYPE_CF32);
    CHECK(b.get_outputs()[0].type == DSP_SAMPLE_TYPE_F32);
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}

///////////////////////////////////////////////////////////////////////////////
// Delay one imag
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp DelayOneImagBlock delays the imaginary part by one sample", "[dsp][utils][delay_one_imag]")
{
    auto in = ndsp_test::randomComplex(1000, 14);
    auto b = std::make_shared<DelayOneImagBlock>();
    // Odd chunk size, so the delay has to carry over buffer boundaries
    auto r = ndsp_test::runSimple<complex_t, complex_t>(b, in, 333);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == in.size());
    CHECK(r.out()[0].real == in[0].real);
    // out[0].imag comes from the uninitialized history value (see report), not asserted
    for (size_t i = 1; i < in.size(); i++)
    {
        REQUIRE(r.out()[i].real == in[i].real);
        REQUIRE(r.out()[i].imag == in[i - 1].imag);
    }
}

TEST_CASE("ndsp DelayOneImagBlock id and config", "[dsp][utils][delay_one_imag]")
{
    DelayOneImagBlock b;
    CHECK(b.d_id == "delay_one_imag_cc");
    CHECK(b.get_cfg_list().empty());
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}

///////////////////////////////////////////////////////////////////////////////
// Exponentiate
///////////////////////////////////////////////////////////////////////////////

namespace
{
    std::vector<complex_t> runExpo(uint32_t expo, const std::vector<complex_t> &in)
    {
        auto b = std::make_shared<ExponentiateBlock>();
        b->set_cfg("expo", expo);
        auto r = ndsp_test::runSimple<complex_t, complex_t>(b, in, 256);
        REQUIRE(r.completed);
        return r.out();
    }

    void checkPow(const std::vector<complex_t> &in, const std::vector<complex_t> &out, int expo)
    {
        REQUIRE(out.size() == in.size());
        for (size_t i = 0; i < in.size(); i++)
        {
            std::complex<double> e = std::pow(std::complex<double>(in[i].real, in[i].imag), expo);
            REQUIRE(std::fabs(out[i].real - e.real()) < 1e-4 * std::max(1.0, std::abs(e)));
            REQUIRE(std::fabs(out[i].imag - e.imag()) < 1e-4 * std::max(1.0, std::abs(e)));
        }
    }
} // namespace

TEST_CASE("ndsp ExponentiateBlock raises samples to an integer power", "[dsp][utils][exponentiate]")
{
    auto in = ndsp_test::randomComplex(1000, 15, 1.2f);
    for (int expo : {2, 3, 4, 8})
    {
        INFO("expo = " << expo);
        checkPow(in, runExpo(expo, in), expo);
    }
}

TEST_CASE("ndsp ExponentiateBlock removes QPSK modulation at power 4", "[dsp][utils][exponentiate]")
{
    // A QPSK symbol at 45 + k*90 degrees becomes -1 at the 4th power
    std::vector<complex_t> in;
    ndsp_test::Lcg rng(16);
    for (int i = 0; i < 400; i++)
        in.push_back(complex_t(rng.sign() * M_SQRT1_2, rng.sign() * M_SQRT1_2));
    auto out = runExpo(4, in);
    REQUIRE(out.size() == in.size());
    for (auto &v : out)
    {
        REQUIRE(std::fabs(v.real + 1.0f) < 1e-5);
        REQUIRE(std::fabs(v.imag) < 1e-5);
    }
}

// Bug : exponentiate.cpp:24-25 always squares first and multiplies from i = 2,
// so expo = 1 (and 0) still produce x^2 instead of x (and 1)
TEST_CASE("ndsp ExponentiateBlock with expo 1 is the identity", "[dsp][utils][exponentiate][!shouldfail]")
{
    auto in = ndsp_test::randomComplex(64, 17);
    checkPow(in, runExpo(1, in), 1);
}

TEST_CASE("ndsp ExponentiateBlock applies a new exponent while running", "[dsp][utils][exponentiate]")
{
    auto b = std::make_shared<ExponentiateBlock>();
    auto in = std::make_shared<DSPStream>(4);
    b->set_input({"in", DSP_SAMPLE_TYPE_CF32, in}, 0);
    ndsp_test::Collector<complex_t> col(b->get_output(0, 4).fifo);
    b->start();

    std::vector<complex_t> x = {{0, 2}};
    ndsp_test::pushSamples(*in, x);
    REQUIRE(col.waitFor(1));
    b->set_cfg("expo", 3);
    ndsp_test::pushSamples(*in, x);
    REQUIRE(col.waitFor(2));
    ndsp_test::pushTerminator(*in);
    REQUIRE(ndsp_test::stopBlock(b));
    col.finish();

    auto d = col.data();
    REQUIRE(d.size() == 2);
    // (2j)^2 = -4, (2j)^3 = -8j
    CHECK(d[0].real == -4.0f);
    CHECK(d[0].imag == 0.0f);
    CHECK(d[1].real == 0.0f);
    CHECK(d[1].imag == -8.0f);
}

TEST_CASE("ndsp ExponentiateBlock id and config", "[dsp][utils][exponentiate]")
{
    ExponentiateBlock b;
    // Note the typo in the id, kept as-is as saved flowgraphs refer to it
    CHECK(b.d_id == "exponantiate_cc");
    CHECK(b.get_cfg("expo") == 2);
    b.set_cfg("expo", 6);
    CHECK(b.get_cfg("expo") == 6);
    CHECK(b.get_cfg_list()["expo"]["type"] == "uint");
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}

///////////////////////////////////////////////////////////////////////////////
// Repeat
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp RepeatBlock repeats every sample", "[dsp][utils][repeat]")
{
    auto b = std::make_shared<RepeatBlock<float>>();
    CHECK(b->get_cfg("interpolation") == 2);
    b->set_cfg("interpolation", 3);
    auto r = ndsp_test::runSimple<float, float>(b, {1, 2, 3, 4, 5}, 2);
    REQUIRE(r.completed);
    CHECK(r.out() == std::vector<float>{1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5});
    CHECK(r.outputs[0].sizes == std::vector<uint32_t>{6, 6, 3});
    CHECK(r.outputs[0].propagating_terminators == 1);
}

TEST_CASE("ndsp RepeatBlock works for complex and int8 samples", "[dsp][utils][repeat]")
{
    auto bc = std::make_shared<RepeatBlock<complex_t>>();
    auto rc = ndsp_test::runSimple<complex_t, complex_t>(bc, {{1, -1}, {2, -2}}, 8);
    REQUIRE(rc.completed);
    REQUIRE(rc.out().size() == 4);
    CHECK(rc.out()[1].real == 1.0f);
    CHECK(rc.out()[1].imag == -1.0f);
    CHECK(rc.out()[2].real == 2.0f);

    auto bh = std::make_shared<RepeatBlock<int8_t>>();
    bh->set_cfg("interpolation", 1);
    auto rh = ndsp_test::runSimple<int8_t, int8_t>(bh, {-128, 0, 127}, 8);
    REQUIRE(rh.completed);
    CHECK(rh.out() == std::vector<int8_t>{-128, 0, 127});
}

TEST_CASE("ndsp RepeatBlock ids and config", "[dsp][utils][repeat]")
{
    CHECK(RepeatBlock<float>().d_id == "repeat_ff");
    CHECK(RepeatBlock<complex_t>().d_id == "repeat_cc");
    CHECK(RepeatBlock<int8_t>().d_id == "repeat_hh");
    RepeatBlock<float> b;
    CHECK(b.get_cfg_list()["interpolation"]["type"] == "int");
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}
