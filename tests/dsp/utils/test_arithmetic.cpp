#include "../ndsp_test_helpers.h"
#include "dsp/utils/add.h"
#include "dsp/utils/multiply.h"
#include "dsp/utils/multiply_const.h"
#include "dsp/utils/subtract.h"
#include <catch2/catch_test_macros.hpp>

using namespace satdump::ndsp;

namespace
{
    template <typename T>
    std::vector<T> elementwise(const std::vector<T> &a, const std::vector<T> &b, int op)
    {
        std::vector<T> r(a.size());
        for (size_t i = 0; i < a.size(); i++)
        {
            T x = a[i], y = b[i];
            r[i] = op == 0 ? x + y : (op == 1 ? x - y : x * y);
        }
        return r;
    }

    // Two-input blocks drop mismatched buffer pairs, but still send an empty buffer
    template <typename BlockT>
    void checkMismatchedSizes()
    {
        auto b = std::make_shared<BlockT>();
        auto ia = std::make_shared<DSPStream>(4);
        auto ib = std::make_shared<DSPStream>(4);
        b->set_input({"a", getTypeSampleType<float>(), ia}, 0);
        b->set_input({"b", getTypeSampleType<float>(), ib}, 1);
        ndsp_test::Collector<float> col(b->get_output(0, 4).fifo);
        b->start();
        ndsp_test::pushSamples(*ia, std::vector<float>{1, 2, 3});
        ndsp_test::pushSamples(*ib, std::vector<float>{1, 2});
        ndsp_test::pushSamples(*ia, std::vector<float>{4});
        ndsp_test::pushSamples(*ib, std::vector<float>{5});
        ndsp_test::pushTerminator(*ia);
        ndsp_test::pushTerminator(*ib);
        REQUIRE(ndsp_test::stopBlock(b));
        REQUIRE(col.waitForTerminator());
        col.finish();
        CHECK(col.sizes() == std::vector<uint32_t>{0, 1});
        CHECK(col.data().size() == 1);
    }

    template <typename BlockT>
    void checkNoConfig()
    {
        BlockT b;
        CHECK(b.get_cfg_list().empty());
        CHECK_THROWS(b.get_cfg("anything"));
        CHECK_THROWS(b.set_cfg("anything", 1));
        Block &bb = b;
        CHECK(bb.set_cfg(nlohmann::json{{"anything", 1}}) == Block::RES_ERR);
        CHECK(bb.get_cfg().is_null());
    }
} // namespace

TEST_CASE("ndsp AddBlock sums two float streams sample-exactly", "[dsp][utils][add]")
{
    auto a = ndsp_test::randomFloats(5000, 1, 100);
    auto c = ndsp_test::randomFloats(5000, 2, 100);
    auto b = std::make_shared<AddBlock<float>>();
    auto r = ndsp_test::runBlock<float, float>(b, {a, c}, 777);
    REQUIRE(r.completed);
    CHECK(r.out() == elementwise(a, c, 0));
    CHECK(r.outputs[0].propagating_terminators == 1);
}

TEST_CASE("ndsp AddBlock sums two complex streams", "[dsp][utils][add]")
{
    auto a = ndsp_test::randomComplex(3000, 3);
    auto c = ndsp_test::randomComplex(3000, 4);
    auto b = std::make_shared<AddBlock<complex_t>>();
    auto r = ndsp_test::runBlock<complex_t, complex_t>(b, {a, c}, 1000);
    REQUIRE(r.completed);
    auto expected = elementwise(a, c, 0);
    REQUIRE(r.out().size() == expected.size());
    for (size_t i = 0; i < expected.size(); i++)
        REQUIRE(ndsp_test::sameBits(r.out()[i], expected[i]));
}

TEST_CASE("ndsp AddBlock ids, IO and config", "[dsp][utils][add]")
{
    AddBlock<float> f;
    AddBlock<complex_t> c;
    CHECK(f.d_id == "add_f");
    CHECK(c.d_id == "add_c");
    REQUIRE(f.get_inputs().size() == 2);
    REQUIRE(f.get_outputs().size() == 1);
    CHECK(f.get_inputs()[1].type == DSP_SAMPLE_TYPE_F32);
    CHECK(c.get_outputs()[0].type == DSP_SAMPLE_TYPE_CF32);
    checkNoConfig<AddBlock<float>>();
}

TEST_CASE("ndsp AddBlock outputs an empty buffer for mismatched sizes", "[dsp][utils][add]")
{
    checkMismatchedSizes<AddBlock<float>>();
}

TEST_CASE("ndsp SubtractBlock computes in1 - in2", "[dsp][utils][subtract]")
{
    auto a = ndsp_test::randomFloats(4096, 5, 10);
    auto c = ndsp_test::randomFloats(4096, 6, 10);
    auto b = std::make_shared<SubtractBlock<float>>();
    auto r = ndsp_test::runBlock<float, float>(b, {a, c}, 512);
    REQUIRE(r.completed);
    CHECK(r.out() == elementwise(a, c, 1));

    auto bc = std::make_shared<SubtractBlock<complex_t>>();
    auto rc = ndsp_test::runBlock<complex_t, complex_t>(bc, {{{1, 2}, {3, 4}}, {{0.5, 5}, {3, -4}}}, 2);
    REQUIRE(rc.completed);
    REQUIRE(rc.out().size() == 2);
    CHECK(rc.out()[0].real == 0.5f);
    CHECK(rc.out()[0].imag == -3.0f);
    CHECK(rc.out()[1].real == 0.0f);
    CHECK(rc.out()[1].imag == 8.0f);
}

TEST_CASE("ndsp SubtractBlock ids, config and mismatched sizes", "[dsp][utils][subtract]")
{
    CHECK(SubtractBlock<float>().d_id == "subtract_f");
    CHECK(SubtractBlock<complex_t>().d_id == "subtract_c");
    checkNoConfig<SubtractBlock<complex_t>>();
    checkMismatchedSizes<SubtractBlock<float>>();
}

TEST_CASE("ndsp MultiplyBlock multiplies floats", "[dsp][utils][multiply]")
{
    auto a = ndsp_test::randomFloats(2048, 7, 3);
    auto c = ndsp_test::randomFloats(2048, 8, 3);
    auto b = std::make_shared<MultiplyBlock<float>>();
    auto r = ndsp_test::runBlock<float, float>(b, {a, c}, 300);
    REQUIRE(r.completed);
    CHECK(r.out() == elementwise(a, c, 2));
}

TEST_CASE("ndsp MultiplyBlock performs complex multiplication", "[dsp][utils][multiply]")
{
    // (1+2j)(3+4j) = -5+10j, j*j = -1, (2-1j)(2+1j) = 5
    std::vector<complex_t> a = {{1, 2}, {0, 1}, {2, -1}};
    std::vector<complex_t> c = {{3, 4}, {0, 1}, {2, 1}};
    auto b = std::make_shared<MultiplyBlock<complex_t>>();
    auto r = ndsp_test::runBlock<complex_t, complex_t>(b, {a, c}, 3);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == 3);
    CHECK(r.out()[0].real == -5.0f);
    CHECK(r.out()[0].imag == 10.0f);
    CHECK(r.out()[1].real == -1.0f);
    CHECK(r.out()[1].imag == 0.0f);
    CHECK(r.out()[2].real == 5.0f);
    CHECK(r.out()[2].imag == 0.0f);
}

TEST_CASE("ndsp MultiplyBlock ids, config and mismatched sizes", "[dsp][utils][multiply]")
{
    CHECK(MultiplyBlock<float>().d_id == "multiply_f");
    CHECK(MultiplyBlock<complex_t>().d_id == "multiply_c");
    checkNoConfig<MultiplyBlock<float>>();
    checkMismatchedSizes<MultiplyBlock<float>>();
}

TEST_CASE("ndsp MultiplyConstBlock scales by the constant", "[dsp][utils][multiply_const]")
{
    auto in = ndsp_test::randomFloats(3000, 9, 5);
    auto b = std::make_shared<MultiplyConstBlock<float>>();
    REQUIRE(ndsp_test::configure(*b, {{"const", -2.5}}) == Block::RES_OK);
    auto r = ndsp_test::runSimple<float, float>(b, in, 1024);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == in.size());
    for (size_t i = 0; i < in.size(); i++)
        REQUIRE(r.out()[i] == in[i] * -2.5f);
}

TEST_CASE("ndsp MultiplyConstBlock scales complex samples by a real constant", "[dsp][utils][multiply_const]")
{
    auto b = std::make_shared<MultiplyConstBlock<complex_t>>();
    b->set_cfg("const", 0.5);
    auto r = ndsp_test::runSimple<complex_t, complex_t>(b, {{2, -4}, {1, 3}}, 4);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == 2);
    CHECK(r.out()[0].real == 1.0f);
    CHECK(r.out()[0].imag == -2.0f);
    CHECK(r.out()[1].real == 0.5f);
    CHECK(r.out()[1].imag == 1.5f);
}

TEST_CASE("ndsp MultiplyConstBlock defaults to identity and round-trips config", "[dsp][utils][multiply_const]")
{
    auto b = std::make_shared<MultiplyConstBlock<float>>();
    CHECK(b->get_cfg("const").get<float>() == 1.0f);
    auto r = ndsp_test::runSimple<float, float>(b, {1, -2, 3}, 2);
    REQUIRE(r.completed);
    CHECK(r.out() == std::vector<float>{1, -2, 3});

    b->set_cfg("const", 4.25);
    CHECK(b->get_cfg("const").get<float>() == 4.25f);
    nlohmann::json all = ((Block &)*b).get_cfg();
    CHECK(all.size() == 1);
    CHECK(all["const"].get<float>() == 4.25f);
    CHECK(b->get_cfg_list()["const"]["type"] == "float");
    CHECK_THROWS(b->get_cfg("nope"));
    CHECK_THROWS(b->set_cfg("nope", 1));
}

TEST_CASE("ndsp MultiplyConstBlock ids", "[dsp][utils][multiply_const]")
{
    // Unlike the other utils blocks, there is no underscore before the type
    CHECK(MultiplyConstBlock<float>().d_id == "multiply_constf");
    CHECK(MultiplyConstBlock<complex_t>().d_id == "multiply_constc");
}
