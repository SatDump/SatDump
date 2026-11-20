#include "dsp/legacy/module_wrapper.h"
#include "dsp/legacy/new2old.h"
#include "dsp/legacy/old2new.h"
#include "dsp/ndsp_test_helpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace satdump::ndsp;

///////////////////////////////////////////////////////////////////////////////
// New -> Old
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp New2OldBlock forwards buffers to a legacy stream", "[dsp][legacy][new2old]")
{
    auto b = std::make_shared<New2OldBlock<float>>();
    b->old_stream = std::make_shared<dsp::stream<float>>();
    auto old = b->old_stream;
    auto in = std::make_shared<DSPStream>(4);
    b->set_input({"in", DSP_SAMPLE_TYPE_F32, in}, 0);

    // Legacy reader thread
    std::vector<float> got;
    std::vector<int> sizes;
    std::atomic<bool> reader_done{false};
    std::thread reader(
        [&]()
        {
            while (got.size() < 1000)
            {
                int n = old->read();
                if (n < 0)
                    break;
                got.insert(got.end(), old->readBuf, old->readBuf + n);
                sizes.push_back(n);
                old->flush();
            }
            reader_done = true;
        });

    b->start();
    auto data = ndsp_test::randomFloats(1000, 71);
    for (size_t i = 0; i < data.size(); i += 250)
        ndsp_test::pushSamples<float>(*in, &data[i], 250);
    bool ok = ndsp_test::waitUntil([&]() { return reader_done.load(); });
    if (!ok)
        old->stopReader();
    reader.join();
    REQUIRE(ok);

    ndsp_test::pushTerminator(*in);
    REQUIRE(ndsp_test::stopBlock(b));
    CHECK(got == data);
    CHECK(sizes == std::vector<int>{250, 250, 250, 250});
}

TEST_CASE("ndsp New2OldBlock ids and config", "[dsp][legacy][new2old]")
{
    CHECK(New2OldBlock<complex_t>().d_id == "new2old_c");
    CHECK(New2OldBlock<float>().d_id == "new2old_f");
    CHECK(New2OldBlock<int16_t>().d_id == "new2old_s");
    CHECK(New2OldBlock<int8_t>().d_id == "new2old_h");
    CHECK(New2OldBlock<uint8_t>().d_id == "new2old_b");
    New2OldBlock<float> b;
    CHECK(b.get_outputs().empty());
    CHECK(b.get_cfg_list().empty());
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}

///////////////////////////////////////////////////////////////////////////////
// Old -> New
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp Old2NewBlock forwards a legacy stream and terminates on request", "[dsp][legacy][old2new]")
{
    auto b = std::make_shared<Old2NewBlock<complex_t>>();
    b->old_stream = std::make_shared<dsp::stream<complex_t>>();
    auto old = b->old_stream;
    ndsp_test::Collector<complex_t> col(b->get_output(0, 4).fifo);
    CHECK(b->is_async());
    b->start();

    auto data = ndsp_test::randomComplex(900, 72);
    std::atomic<bool> writer_ok{true};
    std::thread writer(
        [&]()
        {
            for (size_t i = 0; i < data.size() && writer_ok; i += 300)
            {
                memcpy(old->writeBuf, &data[i], 300 * sizeof(complex_t));
                writer_ok = old->swap(300);
            }
        });
    bool ok = col.waitFor(data.size());
    if (!ok)
        old->stopWriter();
    writer.join();
    REQUIRE(ok);

    // Exit flag first, then unblock the legacy read
    b->old_stream_exit = true;
    old->stopReader();
    REQUIRE(ndsp_test::stopBlock(b));
    REQUIRE(col.waitForTerminator());
    col.finish();

    auto d = col.data();
    REQUIRE(d.size() == data.size());
    for (size_t i = 0; i < data.size(); i++)
        REQUIRE(ndsp_test::sameBits(d[i], data[i]));
    CHECK(col.sizes() == std::vector<uint32_t>{300, 300, 300});
    CHECK(col.propagatingTerminators() == 1);
}

TEST_CASE("ndsp Old2NewBlock skips empty legacy buffers", "[dsp][legacy][old2new]")
{
    auto b = std::make_shared<Old2NewBlock<float>>();
    b->old_stream = std::make_shared<dsp::stream<float>>();
    auto old = b->old_stream;
    ndsp_test::Collector<float> col(b->get_output(0, 4).fifo);
    b->start();
    std::thread writer(
        [&]()
        {
            old->swap(0);
            old->writeBuf[0] = 5.0f;
            old->swap(1);
        });
    bool ok = col.waitFor(1);
    if (!ok)
        old->stopWriter();
    writer.join();
    b->old_stream_exit = true;
    old->stopReader();
    REQUIRE(ndsp_test::stopBlock(b));
    col.finish();
    REQUIRE(ok);
    CHECK(col.sizes() == std::vector<uint32_t>{1});
    CHECK(col.data() == std::vector<float>{5.0f});
}

TEST_CASE("ndsp Old2NewBlock ids and config", "[dsp][legacy][old2new]")
{
    CHECK(Old2NewBlock<complex_t>().d_id == "old2new_c");
    CHECK(Old2NewBlock<float>().d_id == "old2new_f");
    Old2NewBlock<float> b;
    CHECK(b.get_inputs().empty());
    CHECK(b.get_cfg_list().empty());
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}

///////////////////////////////////////////////////////////////////////////////
// Module wrapper (configuration only, running it needs registered pipeline modules)
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp ModuleWrapperBlock config round-trip", "[dsp][legacy][module_wrapper]")
{
    ModuleWrapperBlock b;
    CHECK(b.d_id == "module_wrapper");
    CHECK(b.get_inputs()[0].type == DSP_SAMPLE_TYPE_S8);
    CHECK(b.get_outputs()[0].type == DSP_SAMPLE_TYPE_U8);
    b.set_cfg("module", "some_module");
    b.set_cfg("cfg", nlohmann::json{{"a", 1}, {"b", "x"}});
    b.set_cfg("outsize", 4096);
    CHECK(b.set_cfg("stat", 1) == Block::RES_OK);
    CHECK(b.get_cfg("module") == "some_module");
    CHECK(b.get_cfg("cfg") == nlohmann::json{{"a", 1}, {"b", "x"}});
    CHECK(b.get_cfg("outsize") == 4096);
    auto list = b.get_cfg_list();
    CHECK(list.contains("module"));
    CHECK(list.contains("cfg"));
    CHECK(list.contains("outsize"));
    CHECK_FALSE(list["module"].contains("disabled"));
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}

// Bug: module_wrapper.h:49 declares the stat parameter as "status", but
// get_cfg (module_wrapper.h:61) and set_cfg only know "stat", so the generic
// Block::get_cfg() (used by flowgraph saving and the options UI) throws.
TEST_CASE("ndsp ModuleWrapperBlock get_cfg returns every listed parameter", "[dsp][legacy][module_wrapper][!shouldfail]")
{
    ModuleWrapperBlock b;
    nlohmann::json all;
    CHECK_NOTHROW(all = ((Block &)b).get_cfg());
}
