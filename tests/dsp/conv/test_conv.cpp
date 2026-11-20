#include "../ndsp_test_helpers.h"
#include "dsp/conv/char_to_float.h"
#include "dsp/conv/complex_conjugate.h"
#include "dsp/conv/complex_to_float.h"
#include "dsp/conv/complex_to_ifloat.h"
#include "dsp/conv/complex_to_imag.h"
#include "dsp/conv/complex_to_mag.h"
#include "dsp/conv/complex_to_mag_squared.h"
#include "dsp/conv/complex_to_real.h"
#include "dsp/conv/float_to_char.h"
#include "dsp/conv/float_to_complex.h"
#include "dsp/conv/ifloat_to_complex.h"
#include "dsp/conv/real_to_complex.h"
#include "dsp/conv/short_to_float.h"
#include "dsp/conv/uchar_to_float.h"
#include <catch2/catch_test_macros.hpp>

using namespace satdump::ndsp;

namespace
{
    // Checks id, IO types and that unknown config keys throw
    template <typename BlockT>
    void checkIdentity(const std::string &id, std::vector<BlockIOType> in, std::vector<BlockIOType> out)
    {
        BlockT b;
        CHECK(b.d_id == id);
        REQUIRE(b.get_inputs().size() == in.size());
        REQUIRE(b.get_outputs().size() == out.size());
        for (size_t i = 0; i < in.size(); i++)
            CHECK(b.get_inputs()[i].type == in[i]);
        for (size_t i = 0; i < out.size(); i++)
            CHECK(b.get_outputs()[i].type == out[i]);
        CHECK_THROWS(b.get_cfg("not_a_key"));
        CHECK_THROWS(b.set_cfg("not_a_key", 1));
    }

    // Scale parameter round-trip
    template <typename BlockT>
    void checkScaleCfg()
    {
        BlockT b;
        CHECK(b.get_cfg("scale") == 1.0);
        b.set_cfg("scale", 127.5);
        CHECK(b.get_cfg("scale") == 127.5);
        CHECK(b.get_cfg_list()["scale"]["type"] == "float");
        nlohmann::json all = ((Block &)b).get_cfg();
        CHECK(all.size() == 1);
    }
} // namespace

///////////////////////////////////////////////////////////////////////////////
// Complex splitting
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp ComplexToRealBlock and ComplexToImagBlock extract components", "[dsp][conv][complex_to_real][complex_to_imag]")
{
    auto in = ndsp_test::randomComplex(2500, 41, 3);
    auto r1 = ndsp_test::runSimple<complex_t, float>(std::make_shared<ComplexToRealBlock>(), in, 1000);
    auto r2 = ndsp_test::runSimple<complex_t, float>(std::make_shared<ComplexToImagBlock>(), in, 1000);
    REQUIRE(r1.completed);
    REQUIRE(r2.completed);
    REQUIRE(r1.out().size() == in.size());
    REQUIRE(r2.out().size() == in.size());
    for (size_t i = 0; i < in.size(); i++)
    {
        REQUIRE(r1.out()[i] == in[i].real);
        REQUIRE(r2.out()[i] == in[i].imag);
    }
    checkIdentity<ComplexToRealBlock>("complex_to_real_cf", {DSP_SAMPLE_TYPE_CF32}, {DSP_SAMPLE_TYPE_F32});
    checkIdentity<ComplexToImagBlock>("complex_to_imag_cf", {DSP_SAMPLE_TYPE_CF32}, {DSP_SAMPLE_TYPE_F32});
}

TEST_CASE("ndsp ComplexToFloatBlock splits into two outputs", "[dsp][conv][complex_to_float]")
{
    auto in = ndsp_test::randomComplex(1500, 42);
    auto r = ndsp_test::runBlock<complex_t, float>(std::make_shared<ComplexToFloatBlock>(), {in}, 400);
    REQUIRE(r.completed);
    REQUIRE(r.outputs.size() == 2);
    REQUIRE(r.outputs[0].data.size() == in.size());
    REQUIRE(r.outputs[1].data.size() == in.size());
    for (size_t i = 0; i < in.size(); i++)
    {
        REQUIRE(r.outputs[0].data[i] == in[i].real);
        REQUIRE(r.outputs[1].data[i] == in[i].imag);
    }
    CHECK(r.outputs[0].propagating_terminators == 1);
    CHECK(r.outputs[1].propagating_terminators == 1);
    checkIdentity<ComplexToFloatBlock>("complex_to_float_cf", {DSP_SAMPLE_TYPE_CF32}, {DSP_SAMPLE_TYPE_F32, DSP_SAMPLE_TYPE_F32});
}

TEST_CASE("ndsp FloatToComplexBlock merges real and imag", "[dsp][conv][float_to_complex]")
{
    auto re = ndsp_test::randomFloats(1000, 43);
    auto im = ndsp_test::randomFloats(1000, 44);
    auto r = ndsp_test::runBlock<float, complex_t>(std::make_shared<FloatToComplexBlock>(), {re, im}, 300);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == re.size());
    for (size_t i = 0; i < re.size(); i++)
    {
        REQUIRE(r.out()[i].real == re[i]);
        REQUIRE(r.out()[i].imag == im[i]);
    }
    checkIdentity<FloatToComplexBlock>("float_to_complex_fc", {DSP_SAMPLE_TYPE_F32, DSP_SAMPLE_TYPE_F32}, {DSP_SAMPLE_TYPE_CF32});
}

TEST_CASE("ndsp FloatToComplexBlock drops mismatched buffer pairs", "[dsp][conv][float_to_complex]")
{
    auto b = std::make_shared<FloatToComplexBlock>();
    auto ia = std::make_shared<DSPStream>(4);
    auto ib = std::make_shared<DSPStream>(4);
    b->set_input({"re", DSP_SAMPLE_TYPE_F32, ia}, 0);
    b->set_input({"im", DSP_SAMPLE_TYPE_F32, ib}, 1);
    ndsp_test::Collector<complex_t> col(b->get_output(0, 4).fifo);
    b->start();
    ndsp_test::pushSamples(*ia, std::vector<float>{1, 2});
    ndsp_test::pushSamples(*ib, std::vector<float>{1});
    ndsp_test::pushTerminator(*ia);
    ndsp_test::pushTerminator(*ib);
    REQUIRE(ndsp_test::stopBlock(b));
    REQUIRE(col.waitForTerminator());
    col.finish();
    CHECK(col.sizes() == std::vector<uint32_t>{0});
}

TEST_CASE("ndsp RealToComplexBlock sets a zero imaginary part", "[dsp][conv][real_to_complex]")
{
    auto in = ndsp_test::randomFloats(777, 45);
    auto r = ndsp_test::runSimple<float, complex_t>(std::make_shared<RealToComplexBlock>(), in, 100);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == in.size());
    for (size_t i = 0; i < in.size(); i++)
    {
        REQUIRE(r.out()[i].real == in[i]);
        REQUIRE(r.out()[i].imag == 0.0f);
    }
    checkIdentity<RealToComplexBlock>("real_to_complex_fc", {DSP_SAMPLE_TYPE_F32}, {DSP_SAMPLE_TYPE_CF32});
}

///////////////////////////////////////////////////////////////////////////////
// Interleaved floats
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp ComplexToIFloatBlock interleaves I and Q", "[dsp][conv][complex_to_ifloat]")
{
    std::vector<complex_t> in = {{1, 2}, {3, 4}, {5, 6}};
    auto r = ndsp_test::runSimple<complex_t, float>(std::make_shared<ComplexToIFloatBlock>(), in, 2);
    REQUIRE(r.completed);
    CHECK(r.out() == std::vector<float>{1, 2, 3, 4, 5, 6});
    CHECK(r.outputs[0].sizes == std::vector<uint32_t>{4, 2});
    // Output buffers are twice as large as the input ones
    CHECK(r.outputs[0].max_sizes == std::vector<uint32_t>{4, 2});
    checkIdentity<ComplexToIFloatBlock>("complex_to_ifloat_cf", {DSP_SAMPLE_TYPE_CF32}, {DSP_SAMPLE_TYPE_F32});
}

TEST_CASE("ndsp IFloatToComplexBlock deinterleaves across buffer boundaries", "[dsp][conv][ifloat_to_complex]")
{
    std::vector<float> in = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    // Chunks of 3 split pairs between buffers : [1 2 3] [4 5 6] [7 8 9] [10]
    auto r = ndsp_test::runSimple<float, complex_t>(std::make_shared<IFloatToComplexBlock>(), in, 3);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == 5);
    for (int i = 0; i < 5; i++)
    {
        CHECK(r.out()[i].real == (float)(2 * i + 1));
        CHECK(r.out()[i].imag == (float)(2 * i + 2));
    }
    CHECK(r.outputs[0].sizes == std::vector<uint32_t>{1, 2, 1, 1});
    checkIdentity<IFloatToComplexBlock>("ifloat_to_complex_fc", {DSP_SAMPLE_TYPE_F32}, {DSP_SAMPLE_TYPE_CF32});
}

TEST_CASE("ndsp ComplexToIFloatBlock and IFloatToComplexBlock round-trip", "[dsp][conv][complex_to_ifloat][ifloat_to_complex]")
{
    auto in = ndsp_test::randomComplex(3001, 46);
    auto r1 = ndsp_test::runSimple<complex_t, float>(std::make_shared<ComplexToIFloatBlock>(), in, 1000);
    REQUIRE(r1.completed);
    auto r2 = ndsp_test::runSimple<float, complex_t>(std::make_shared<IFloatToComplexBlock>(), r1.out(), 333);
    REQUIRE(r2.completed);
    REQUIRE(r2.out().size() == in.size());
    for (size_t i = 0; i < in.size(); i++)
        REQUIRE(ndsp_test::sameBits(r2.out()[i], in[i]));
}

///////////////////////////////////////////////////////////////////////////////
// Magnitude / conjugate
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp ComplexToMagBlock computes the magnitude", "[dsp][conv][complex_to_mag]")
{
    std::vector<complex_t> in = {{3, 4}, {0, 0}, {-5, 12}, {0, -2}};
    auto more = ndsp_test::randomComplex(1000, 47, 10);
    in.insert(in.end(), more.begin(), more.end());
    auto r = ndsp_test::runSimple<complex_t, float>(std::make_shared<ComplexToMagBlock>(), in, 256);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == in.size());
    CHECK(r.out()[0] == 5.0f);
    CHECK(r.out()[1] == 0.0f);
    CHECK(r.out()[2] == 13.0f);
    CHECK(r.out()[3] == 2.0f);
    for (size_t i = 0; i < in.size(); i++)
        REQUIRE(std::fabs(r.out()[i] - std::hypot(in[i].real, in[i].imag)) < 1e-5f * std::max(1.0f, r.out()[i]));
    checkIdentity<ComplexToMagBlock>("complex_to_mag_cf", {DSP_SAMPLE_TYPE_CF32}, {DSP_SAMPLE_TYPE_F32});
}

TEST_CASE("ndsp ComplexToMagSquaredBlock computes the power", "[dsp][conv][complex_to_mag_squared]")
{
    auto in = ndsp_test::randomComplex(1000, 48, 4);
    in[0] = complex_t(3, -4);
    auto r = ndsp_test::runSimple<complex_t, float>(std::make_shared<ComplexToMagSquaredBlock>(), in, 300);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == in.size());
    CHECK(r.out()[0] == 25.0f);
    for (size_t i = 0; i < in.size(); i++)
    {
        float e = in[i].real * in[i].real + in[i].imag * in[i].imag;
        REQUIRE(std::fabs(r.out()[i] - e) < 1e-5f * std::max(1.0f, e));
    }
    checkIdentity<ComplexToMagSquaredBlock>("complex_to_mag_squared_cf", {DSP_SAMPLE_TYPE_CF32}, {DSP_SAMPLE_TYPE_F32});
}

TEST_CASE("ndsp ComplexConjugateBlock negates the imaginary part", "[dsp][conv][complex_conjugate]")
{
    auto in = ndsp_test::randomComplex(2000, 49);
    auto r = ndsp_test::runSimple<complex_t, complex_t>(std::make_shared<ComplexConjugateBlock>(), in, 512);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == in.size());
    for (size_t i = 0; i < in.size(); i++)
    {
        REQUIRE(r.out()[i].real == in[i].real);
        REQUIRE(r.out()[i].imag == -in[i].imag);
    }
    checkIdentity<ComplexConjugateBlock>("complex_conjugate_cc", {DSP_SAMPLE_TYPE_CF32}, {DSP_SAMPLE_TYPE_CF32});
}

///////////////////////////////////////////////////////////////////////////////
// Integer <-> float
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp CharToFloatBlock divides by scale", "[dsp][conv][char_to_float]")
{
    std::vector<int8_t> in;
    for (int v = -128; v <= 127; v++)
        in.push_back((int8_t)v);

    auto b1 = std::make_shared<CharToFloatBlock>();
    auto r1 = ndsp_test::runSimple<int8_t, float>(b1, in, 100);
    REQUIRE(r1.completed);
    REQUIRE(r1.out().size() == in.size());
    for (size_t i = 0; i < in.size(); i++)
        REQUIRE(r1.out()[i] == (float)in[i]);

    auto b2 = std::make_shared<CharToFloatBlock>();
    b2->set_cfg("scale", 128);
    auto r2 = ndsp_test::runSimple<int8_t, float>(b2, in, 64);
    REQUIRE(r2.completed);
    REQUIRE(r2.out().size() == in.size());
    for (size_t i = 0; i < in.size(); i++)
        REQUIRE(std::fabs(r2.out()[i] - in[i] / 128.0f) < 1e-7f);
    CHECK(r2.out().front() == -1.0f);

    checkIdentity<CharToFloatBlock>("char_to_float_hf", {DSP_SAMPLE_TYPE_S8}, {DSP_SAMPLE_TYPE_F32});
    checkScaleCfg<CharToFloatBlock>();
}

TEST_CASE("ndsp ShortToFloatBlock divides by scale", "[dsp][conv][short_to_float]")
{
    std::vector<int16_t> in = {-32768, -1000, -1, 0, 1, 1000, 32767};
    auto b = std::make_shared<ShortToFloatBlock>();
    b->set_cfg("scale", 32768.0);
    auto r = ndsp_test::runSimple<int16_t, float>(b, in, 3);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == in.size());
    for (size_t i = 0; i < in.size(); i++)
        REQUIRE(std::fabs(r.out()[i] - in[i] / 32768.0f) < 1e-7f);
    CHECK(r.out()[0] == -1.0f);

    // The id uses "h" (int8) although the input is int16 ("s")
    checkIdentity<ShortToFloatBlock>("short_to_float_hf", {DSP_SAMPLE_TYPE_S16}, {DSP_SAMPLE_TYPE_F32});
    checkScaleCfg<ShortToFloatBlock>();
}

TEST_CASE("ndsp UCharToFloatBlock multiplies by scale", "[dsp][conv][uchar_to_float]")
{
    // Unlike the char / short versions, scale is a multiplier here
    std::vector<uint8_t> in;
    for (int v = 0; v < 256; v++)
        in.push_back((uint8_t)v);
    auto b = std::make_shared<UCharToFloatBlock>();
    b->set_cfg("scale", 0.5);
    auto r = ndsp_test::runSimple<uint8_t, float>(b, in, 100);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == in.size());
    for (size_t i = 0; i < in.size(); i++)
        REQUIRE(r.out()[i] == in[i] * 0.5f);

    checkIdentity<UCharToFloatBlock>("uchar_to_float_bf", {DSP_SAMPLE_TYPE_U8}, {DSP_SAMPLE_TYPE_F32});
    checkScaleCfg<UCharToFloatBlock>();
}

TEST_CASE("ndsp FloatToCharBlock scales, rounds and clips", "[dsp][conv][float_to_char]")
{
    std::vector<float> in = {0.0f, 0.24f, -0.26f, 0.5f, -0.5f, 0.9f, 1.0f, -1.0f, 2.0f, -3.0f};
    auto b = std::make_shared<FloatToCharBlock>();
    b->set_cfg("scale", 127.0);
    auto r = ndsp_test::runSimple<float, int8_t>(b, in, 4);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == in.size());
    for (size_t i = 0; i < in.size(); i++)
    {
        float v = std::max(-128.0f, std::min(127.0f, in[i] * 127.0f));
        INFO("in = " << in[i]);
        // Rounding mode differs between VOLK kernels, allow one LSB
        REQUIRE(std::fabs((float)r.out()[i] - v) <= 1.0f);
    }
    CHECK(r.out()[0] == 0);
    CHECK(r.out()[6] == 127);
    CHECK(r.out()[7] == -127);
    CHECK(r.out()[8] == 127);
    CHECK(r.out()[9] == -128);

    checkIdentity<FloatToCharBlock>("float_to_char_fh", {DSP_SAMPLE_TYPE_F32}, {DSP_SAMPLE_TYPE_S8});
    checkScaleCfg<FloatToCharBlock>();
}

TEST_CASE("ndsp FloatToCharBlock and CharToFloatBlock round-trip", "[dsp][conv][float_to_char][char_to_float]")
{
    std::vector<int8_t> in;
    for (int i = 0; i < 1000; i++)
        in.push_back((int8_t)((i * 37) % 255 - 127));
    auto c2f = std::make_shared<CharToFloatBlock>();
    c2f->set_cfg("scale", 127.0);
    auto r1 = ndsp_test::runSimple<int8_t, float>(c2f, in, 128);
    REQUIRE(r1.completed);
    auto f2c = std::make_shared<FloatToCharBlock>();
    f2c->set_cfg("scale", 127.0);
    auto r2 = ndsp_test::runSimple<float, int8_t>(f2c, r1.out(), 100);
    REQUIRE(r2.completed);
    CHECK(r2.out() == in);
}
