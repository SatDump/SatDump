#include "../ndsp_test_helpers.h"
#include "dsp/path/selector.h"
#include "dsp/path/splitter.h"
#include "dsp/path/switch.h"
#include <catch2/catch_test_macros.hpp>

using namespace satdump::ndsp;

///////////////////////////////////////////////////////////////////////////////
// Splitter
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp SplitterBlock copies its input to every output", "[dsp][path][splitter]")
{
    auto b = std::make_shared<SplitterBlock<float>>();
    CHECK(b->get_outputs().empty());
    REQUIRE(ndsp_test::configure(*b, {{"noutputs", 3}}) == Block::RES_IOUPD);
    REQUIRE(b->get_outputs().size() == 3);
    CHECK(b->get_outputs()[2].name == "out3");

    auto in = ndsp_test::randomFloats(5000, 51);
    auto r = ndsp_test::runSimple<float, float>(b, in, 1000);
    REQUIRE(r.completed);
    REQUIRE(r.outputs.size() == 3);
    for (auto &o : r.outputs)
    {
        CHECK(o.data == in);
        CHECK(o.propagating_terminators == 1);
    }
}

TEST_CASE("ndsp SplitterBlock works for every sample type", "[dsp][path][splitter]")
{
    auto bc = std::make_shared<SplitterBlock<complex_t>>();
    bc->set_cfg("noutputs", 2);
    auto inc = ndsp_test::randomComplex(300, 52);
    auto rc = ndsp_test::runSimple<complex_t, complex_t>(bc, inc, 128);
    REQUIRE(rc.completed);
    for (auto &o : rc.outputs)
    {
        REQUIRE(o.data.size() == inc.size());
        for (size_t i = 0; i < inc.size(); i++)
            REQUIRE(ndsp_test::sameBits(o.data[i], inc[i]));
    }

    auto bs = std::make_shared<SplitterBlock<int16_t>>();
    bs->set_cfg("noutputs", 2);
    std::vector<int16_t> ins = {-32768, 0, 32767, 5};
    auto rs = ndsp_test::runSimple<int16_t, int16_t>(bs, ins, 3);
    REQUIRE(rs.completed);
    CHECK(rs.outputs[0].data == ins);
    CHECK(rs.outputs[1].data == ins);

    auto bh = std::make_shared<SplitterBlock<int8_t>>();
    bh->set_cfg("noutputs", 1);
    auto rh = ndsp_test::runSimple<int8_t, int8_t>(bh, {-1, 2, -3}, 3);
    REQUIRE(rh.completed);
    CHECK(rh.outputs[0].data == std::vector<int8_t>{-1, 2, -3});

    auto bb = std::make_shared<SplitterBlock<uint8_t>>();
    bb->set_cfg("noutputs", 2);
    auto rb = ndsp_test::runSimple<uint8_t, uint8_t>(bb, {255, 0, 7}, 2);
    REQUIRE(rb.completed);
    CHECK(rb.outputs[1].data == std::vector<uint8_t>{255, 0, 7});
}

TEST_CASE("ndsp SplitterBlock noutputs config", "[dsp][path][splitter]")
{
    SplitterBlock<float> b;
    Block &bb = b;
    CHECK(b.d_id == "splitter_ff");
    CHECK(SplitterBlock<complex_t>().d_id == "splitter_cc");
    CHECK(b.get_cfg("noutputs") == 0);

    CHECK(bb.set_cfg(nlohmann::json{{"noutputs", 2}}) == Block::RES_IOUPD);
    auto first = b.get_outputs();
    REQUIRE(first.size() == 2);
    CHECK(first[0].fifo != nullptr);
    CHECK(b.get_output_by_id("1").fifo == first[0].fifo);
    CHECK(b.get_output_by_id("2").fifo == first[1].fifo);

    // Same count keeps the existing outputs
    b.set_cfg("noutputs", 2);
    CHECK(b.get_outputs()[0].fifo == first[0].fifo);

    // Negative values are clamped to 0
    b.set_cfg("noutputs", -4);
    CHECK(b.get_cfg("noutputs") == 0);
    CHECK(b.get_outputs().empty());

    auto list = b.get_cfg_list();
    CHECK(list["noutputs"]["type"] == "int");
    CHECK(list["noutputs"]["disable"] == false);
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
    CHECK_THROWS(b.get_output_by_id("1"));
}

TEST_CASE("ndsp SplitterBlock add_output and del_output", "[dsp][path][splitter]")
{
    SplitterBlock<float> b;
    auto &o = b.add_output("vfo_a");
    CHECK(o.name == "vfo_a");
    CHECK(o.type == DSP_SAMPLE_TYPE_F32);
    REQUIRE(o.fifo != nullptr);
    auto fifo_a = o.fifo;
    b.add_output("vfo_b", false);
    REQUIRE(b.get_outputs().size() == 2);
    CHECK(b.get_output_by_id("vfo_b").name == "vfo_b");

    // Deleting with a terminator posts one on the removed output
    b.del_output("vfo_a", true);
    REQUIRE(b.get_outputs().size() == 1);
    CHECK(b.get_outputs()[0].name == "vfo_b");
    DSPBuffer t;
    REQUIRE(fifo_a->try_dequeue(t));
    CHECK(t.terminatorShouldPropagate());
    fifo_a->free(t);

    b.del_output("vfo_b", false);
    CHECK(b.get_outputs().empty());
    b.del_output("unknown", true); // No-op
}

TEST_CASE("ndsp SplitterBlock does not forward terminators to non-forwarding outputs", "[dsp][path][splitter]")
{
    auto b = std::make_shared<SplitterBlock<float>>();
    b->add_output("main", true);
    b->add_output("vfo", false);
    auto in = std::make_shared<DSPStream>(4);
    b->set_input({"in", DSP_SAMPLE_TYPE_F32, in}, 0);
    ndsp_test::Collector<float> c_main(b->get_output_by_id("main").fifo);
    ndsp_test::Collector<float> c_vfo(b->get_output_by_id("vfo").fifo);
    b->start();
    CHECK(b->get_cfg_list()["noutputs"]["disable"] == true);
    ndsp_test::pushSamples(*in, std::vector<float>{1, 2, 3});
    ndsp_test::pushTerminator(*in);
    REQUIRE(ndsp_test::stopBlock(b));
    REQUIRE(c_main.waitForTerminator());
    c_main.finish();
    c_vfo.finish();
    CHECK(c_main.data() == std::vector<float>{1, 2, 3});
    CHECK(c_vfo.data() == std::vector<float>{1, 2, 3});
    CHECK(c_main.terminators() == 1);
    CHECK(c_vfo.terminators() == 0);
}

TEST_CASE("ndsp SplitterBlock non-propagating terminator is not forwarded", "[dsp][path][splitter]")
{
    auto b = std::make_shared<SplitterBlock<float>>();
    b->set_cfg("noutputs", 2);
    auto r = ndsp_test::runSimple<float, float>(b, {1, 2}, 2, 2);
    REQUIRE(r.completed);
    CHECK(r.outputs[0].terminators == 0);
    CHECK(r.outputs[1].terminators == 0);
    CHECK(r.outputs[1].data == std::vector<float>{1, 2});
}

///////////////////////////////////////////////////////////////////////////////
// Switch
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp SwitchBlock routes to the active output only", "[dsp][path][switch]")
{
    auto b = std::make_shared<SwitchBlock<float>>();
    REQUIRE(ndsp_test::configure(*b, {{"noutputs", 3}}) == Block::RES_IOUPD);
    REQUIRE(ndsp_test::configure(*b, {{"active_id", 1}}) == Block::RES_OK);
    auto in = ndsp_test::randomFloats(2000, 53);
    auto r = ndsp_test::runSimple<float, float>(b, in, 500);
    REQUIRE(r.completed);
    REQUIRE(r.outputs.size() == 3);
    CHECK(r.outputs[0].data.empty());
    CHECK(r.outputs[1].data == in);
    CHECK(r.outputs[2].data.empty());
    // But the terminator goes everywhere
    for (auto &o : r.outputs)
        CHECK(o.propagating_terminators == 1);
}

TEST_CASE("ndsp SwitchBlock can switch output while running", "[dsp][path][switch]")
{
    auto b = std::make_shared<SwitchBlock<complex_t>>();
    b->set_cfg("noutputs", 2);
    auto in = std::make_shared<DSPStream>(4);
    b->set_input({"in", DSP_SAMPLE_TYPE_CF32, in}, 0);
    ndsp_test::Collector<complex_t> c0(b->get_output(0, 4).fifo);
    ndsp_test::Collector<complex_t> c1(b->get_output(1, 4).fifo);
    b->start();

    ndsp_test::pushSamples(*in, std::vector<complex_t>{{1, 1}, {2, 2}});
    REQUIRE(c0.waitFor(2));
    b->set_cfg("active_id", 1);
    ndsp_test::pushSamples(*in, std::vector<complex_t>{{3, 3}});
    REQUIRE(c1.waitFor(1));
    ndsp_test::pushTerminator(*in);
    REQUIRE(ndsp_test::stopBlock(b));
    REQUIRE(c0.waitForTerminator());
    REQUIRE(c1.waitForTerminator());
    c0.finish();
    c1.finish();
    REQUIRE(c0.data().size() == 2);
    REQUIRE(c1.data().size() == 1);
    CHECK(c0.data()[1].imag == 2.0f);
    CHECK(c1.data()[0].real == 3.0f);
}

TEST_CASE("ndsp SwitchBlock config and active_id safety", "[dsp][path][switch]")
{
    SwitchBlock<float> b;
    CHECK(b.d_id == "switch_ff");
    CHECK(SwitchBlock<complex_t>().d_id == "switch_cc");
    CHECK(SwitchBlock<uint8_t>().d_id == "switch_bb");
    // A fresh switch already has one output, although noutputs reports 0
    CHECK(b.get_outputs().size() == 1);
    CHECK(b.get_cfg("noutputs") == 0);
    CHECK(b.get_cfg("active_id") == 0);

    b.set_cfg("noutputs", 4);
    REQUIRE(b.get_outputs().size() == 4);
    CHECK(b.get_outputs()[3].name == "out4");
    b.set_cfg("active_id", 3);
    CHECK(b.get_cfg("active_id") == 3);

    // Out of range resets to 0
    b.set_cfg("active_id", 4);
    CHECK(b.get_cfg("active_id") == 0);
    b.set_cfg("active_id", -1);
    CHECK(b.get_cfg("active_id") == 0);

    // Reducing the output count also resets an out of range active_id
    b.set_cfg("active_id", 3);
    CHECK(b.set_cfg("noutputs", 2) == Block::RES_IOUPD);
    CHECK(b.get_cfg("active_id") == 0);
    b.set_cfg("noutputs", -1);
    CHECK(b.get_cfg("noutputs") == 0);
    CHECK(b.get_outputs().empty());

    auto list = b.get_cfg_list();
    CHECK(list.contains("noutputs"));
    CHECK(list.contains("active_id"));
    CHECK(list["noutputs"]["disable"] == false);
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}

///////////////////////////////////////////////////////////////////////////////
// Selector
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp SelectorBlock forwards the active input only", "[dsp][path][selector]")
{
    auto b = std::make_shared<SelectorBlock<float>>();
    REQUIRE(ndsp_test::configure(*b, {{"ninputs", 3}}) == Block::RES_IOUPD);
    REQUIRE(ndsp_test::configure(*b, {{"active_id", 2}}) == Block::RES_OK);
    REQUIRE(b->get_inputs().size() == 3);

    std::vector<std::shared_ptr<DSPStream>> ins;
    for (int i = 0; i < 3; i++)
    {
        ins.push_back(std::make_shared<DSPStream>(4));
        b->set_input({"in", DSP_SAMPLE_TYPE_F32, ins[i]}, i);
    }
    ndsp_test::Collector<float> col(b->get_output(0, 4).fifo);
    b->start();

    // Inactive inputs are consumed and dropped
    for (int k = 0; k < 10; k++)
    {
        ndsp_test::pushSamples(*ins[0], std::vector<float>{-1, -1});
        ndsp_test::pushSamples(*ins[1], std::vector<float>{-2});
        ndsp_test::pushSamples(*ins[2], std::vector<float>{(float)k, (float)k + 0.5f});
    }
    REQUIRE(col.waitFor(20));
    ndsp_test::pushTerminator(*ins[2]);
    REQUIRE(ndsp_test::stopBlock(b));
    REQUIRE(col.waitForTerminator());
    col.finish();

    auto d = col.data();
    REQUIRE(d.size() == 20);
    for (int k = 0; k < 10; k++)
    {
        CHECK(d[2 * k] == (float)k);
        CHECK(d[2 * k + 1] == (float)k + 0.5f);
    }
}

TEST_CASE("ndsp SelectorBlock stops on a terminator from any input", "[dsp][path][selector]")
{
    auto b = std::make_shared<SelectorBlock<complex_t>>();
    b->set_cfg("ninputs", 2);
    auto i0 = std::make_shared<DSPStream>(4);
    auto i1 = std::make_shared<DSPStream>(4);
    b->set_input({"in", DSP_SAMPLE_TYPE_CF32, i0}, 0);
    b->set_input({"in", DSP_SAMPLE_TYPE_CF32, i1}, 1);
    ndsp_test::Collector<complex_t> col(b->get_output(0, 4).fifo);
    b->start();
    // Terminator on the inactive input
    ndsp_test::pushTerminator(*i1);
    REQUIRE(ndsp_test::stopBlock(b));
    REQUIRE(col.waitForTerminator());
    col.finish();
    CHECK(col.data().empty());
    CHECK(col.propagatingTerminators() == 1);
}

TEST_CASE("ndsp SelectorBlock non-propagating terminator", "[dsp][path][selector]")
{
    auto b = std::make_shared<SelectorBlock<float>>();
    b->set_cfg("ninputs", 1);
    auto i0 = std::make_shared<DSPStream>(4);
    b->set_input({"in", DSP_SAMPLE_TYPE_F32, i0}, 0);
    ndsp_test::Collector<float> col(b->get_output(0, 4).fifo);
    b->start();
    ndsp_test::pushSamples(*i0, std::vector<float>{7});
    ndsp_test::pushTerminator(*i0, false);
    REQUIRE(ndsp_test::stopBlock(b));
    col.finish();
    CHECK(col.data() == std::vector<float>{7});
    CHECK(col.terminators() == 0);
}

TEST_CASE("ndsp SelectorBlock config and active_id safety", "[dsp][path][selector]")
{
    SelectorBlock<float> b;
    CHECK(b.d_id == "selector_ff");
    CHECK(SelectorBlock<complex_t>().d_id == "selector_cc");
    CHECK(SelectorBlock<int16_t>().d_id == "selector_ss");
    // Like the switch, a fresh selector has one input while ninputs reports 0
    CHECK(b.get_inputs().size() == 1);
    CHECK(b.get_cfg("ninputs") == 0);
    CHECK(b.get_outputs().size() == 1);

    b.set_cfg("ninputs", 3);
    REQUIRE(b.get_inputs().size() == 3);
    CHECK(b.get_inputs()[2].name == "in3");
    b.set_cfg("active_id", 2);
    CHECK(b.get_cfg("active_id") == 2);
    b.set_cfg("active_id", 3);
    CHECK(b.get_cfg("active_id") == 0);

    b.set_cfg("active_id", 2);
    CHECK(b.set_cfg("ninputs", 1) == Block::RES_IOUPD);
    CHECK(b.get_cfg("active_id") == 0);
    b.set_cfg("ninputs", -2);
    CHECK(b.get_cfg("ninputs") == 0);
    CHECK(b.get_inputs().empty());
    // Without inputs it counts as a source
    CHECK(b.is_async());

    auto list = b.get_cfg_list();
    CHECK(list["ninputs"]["disable"] == false);
    CHECK(list.contains("active_id"));
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}
