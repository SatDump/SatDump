#include "dsp/flowgraph/dsp_flowgraph_register.h"
#include "dsp/flowgraph/flowgraph.h"
#include "dsp/flowgraph/missing_node.h"
#include "dsp/flowgraph/node_int.h"
#include "dsp/io/file_sink.h"
#include "dsp/io/file_source.h"
#include "dsp/io/waveform.h"
#include "dsp/ndsp_test_helpers.h"
#include "dsp/utils/add.h"
#include "dsp/utils/multiply_const.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <set>

using Catch::Matchers::WithinAbs;
using namespace satdump::ndsp;
using namespace satdump::ndsp::flowgraph;

namespace
{
    // Registers the few nodes the tests use (registerNodesInFlowgraph needs
    // resources and GUI widgets, so it is not used here)
    void registerTestNodes(Flowgraph &fg)
    {
        registerNodeSimple<FileSourceBlock<float>>(fg, "IO/File Source F");
        registerNodeSimple<FileSinkBlock<float>>(fg, "IO/File Sink F");
        registerNodeSimple<MultiplyConstBlock<float>>(fg, "Utils/Multiply Const FF");
        registerNodeSimple<AddBlock<float>>(fg, "Utils/Add FF");
        registerNodeSimple<WaveformBlock<float>>(fg, "IO/Waveform F");
    }

    std::shared_ptr<Flowgraph::Node> addBlock(Flowgraph &fg, const std::string &id, nlohmann::json cfg = {})
    {
        auto i = fg.node_internal_registry.at(id).inst(&fg);
        if (!cfg.is_null())
            i->setP(cfg);
        return fg.addNode(id, i);
    }

    // IO ids of a serialized node, inputs or outputs, in order
    std::vector<int> ioIds(const nlohmann::json &node, bool out)
    {
        std::vector<int> r;
        for (auto &io : node["io"])
            if (io["is_out"].get<bool>() == out)
                r.push_back(io["id"].get<int>());
        return r;
    }

    nlohmann::json link(int id, int start, int end) { return {{"id", id}, {"start", start}, {"end", end}}; }

    // Runs a flowgraph with a timeout. The flowgraph is leaked on timeout.
    bool runFlowgraph(std::shared_ptr<Flowgraph> fg, std::function<void()> while_running = nullptr)
    {
        auto done = std::make_shared<std::atomic<bool>>(false);
        std::thread th(
            [fg, done]()
            {
                fg->run();
                *done = true;
            });
        if (while_running)
            while_running();
        bool ok = ndsp_test::waitUntil([&]() { return done->load(); });
        if (!ok)
        {
            ndsp_test::leakForever(fg);
            th.detach();
            return false;
        }
        th.join();
        return true;
    }
} // namespace

TEST_CASE("ndsp Flowgraph registerNodeSimple uses the block id as key", "[dsp][flowgraph][register]")
{
    Flowgraph fg;
    registerTestNodes(fg);
    REQUIRE(fg.node_internal_registry.count("file_source_f") == 1);
    REQUIRE(fg.node_internal_registry.count("multiply_constf") == 1);
    CHECK(fg.node_internal_registry["file_sink_f"].menuname == "IO/File Sink F");

    auto n = fg.node_internal_registry["add_f"].inst(&fg);
    REQUIRE(n != nullptr);
    REQUIRE(n->blk != nullptr);
    CHECK(n->blk->d_id == "add_f");
    CHECK_FALSE(n->is_errored());
    // Registering again keeps the first entry
    registerNodeSimple<AddBlock<float>>(fg, "Other/Name");
    CHECK(fg.node_internal_registry["add_f"].menuname == "Utils/Add FF");
}

TEST_CASE("ndsp Flowgraph registerNode registers custom NodeInternal classes", "[dsp][flowgraph][register]")
{
    struct CustomNode : public NodeInternal
    {
        CustomNode(const Flowgraph *f) : NodeInternal(f, std::make_shared<MultiplyConstBlock<complex_t>>()) {}
        nlohmann::json getP() override
        {
            auto p = NodeInternal::getP();
            p["extra"] = 1;
            return p;
        }
    };
    Flowgraph fg;
    registerNode<CustomNode>(fg, "Custom/Node");
    REQUIRE(fg.node_internal_registry.count("multiply_constc") == 1);
    auto n = fg.node_internal_registry["multiply_constc"].inst(&fg);
    CHECK(n->getP()["extra"] == 1);
    CHECK(n->getP()["const"] == 1.0);
}

TEST_CASE("ndsp NodeInternal getP and setP go through the block config", "[dsp][flowgraph][node_int]")
{
    Flowgraph fg;
    auto blk = std::make_shared<MultiplyConstBlock<float>>();
    NodeInternal n(&fg, blk);
    CHECK(n.getP() == nlohmann::json{{"const", 1.0}});
    n.setP({{"const", 3.0}});
    CHECK(blk->get_cfg("const") == 3.0);
    CHECK(n.getP()["const"] == 3.0);
    n.upd_state();
}

TEST_CASE("ndsp Flowgraph nodes get unique ids and IO ids", "[dsp][flowgraph]")
{
    Flowgraph fg;
    registerTestNodes(fg);
    addBlock(fg, "file_source_f");
    addBlock(fg, "add_f");
    addBlock(fg, "file_sink_f");

    nlohmann::json j = fg.getJSON();
    REQUIRE(j["nodes"].size() == 3);
    std::set<int> node_ids, io_ids;
    size_t io_count = 0;
    for (auto &n : j["nodes"])
    {
        node_ids.insert(n["id"].get<int>());
        for (auto &io : n["io"])
        {
            io_ids.insert(io["id"].get<int>());
            io_count++;
        }
    }
    CHECK(node_ids == std::set<int>{0, 1, 2});
    // source : 1 out, add : 2 in + 1 out, sink : 1 in
    CHECK(io_count == 5);
    CHECK(io_ids.size() == io_count);

    // Node JSON content
    auto &add = j["nodes"][1];
    CHECK(add["int_id"] == "add_f");
    CHECK(ioIds(add, false).size() == 2);
    CHECK(ioIds(add, true).size() == 1);
    CHECK(add["io"][0]["name"] == "in 1");
    CHECK(add["disabled"] == false);
    CHECK(j["links"].empty());
}

TEST_CASE("ndsp Flowgraph JSON save and load round-trip", "[dsp][flowgraph]")
{
    Flowgraph fg;
    registerTestNodes(fg);
    auto src = addBlock(fg, "file_source_f", {{"file", "/tmp/in.f32"}, {"buffer_size", 512}});
    auto mul = addBlock(fg, "multiply_constf", {{"const", 2.0}});
    auto sink = addBlock(fg, "file_sink_f", {{"file", "/tmp/out.f32"}});
    fg.variables["gain"] = 4.5;

    nlohmann::json j = fg.getJSON();
    int src_out = ioIds(j["nodes"][0], true)[0];
    int mul_in = ioIds(j["nodes"][1], false)[0];
    int mul_out = ioIds(j["nodes"][1], true)[0];
    int sink_in = ioIds(j["nodes"][2], false)[0];
    j["links"] = {link(0, src_out, mul_in), link(1, mul_out, sink_in)};
    j["nodes"][1]["pos_x"] = 12.5;
    j["nodes"][1]["pos_y"] = -3;
    j["nodes"][2]["disabled"] = true;

    Flowgraph fg2;
    registerTestNodes(fg2);
    fg2.setJSON(j);
    nlohmann::json j2 = fg2.getJSON();
    CHECK(j2 == j);
    CHECK(fg2.variables["gain"] == 4.5);
    CHECK(j2["nodes"][0]["int_cfg"]["file"] == "/tmp/in.f32");
    CHECK(j2["nodes"][0]["int_cfg"]["buffer_size"] == 512);
    CHECK(j2["nodes"][1]["int_cfg"]["const"] == 2.0);
    CHECK(j2["links"].size() == 2);

    // Loading again replaces everything
    fg2.setJSON(j);
    CHECK(fg2.getJSON() == j);
}

TEST_CASE("ndsp Flowgraph setJSON drops links to unknown IOs", "[dsp][flowgraph]")
{
    Flowgraph fg;
    registerTestNodes(fg);
    addBlock(fg, "file_source_f");
    addBlock(fg, "file_sink_f");
    nlohmann::json j = fg.getJSON();
    int out = ioIds(j["nodes"][0], true)[0];
    int in = ioIds(j["nodes"][1], false)[0];
    j["links"] = {link(0, out, in), link(1, out, 999), link(2, 998, in)};
    fg.setJSON(j);
    nlohmann::json j2 = fg.getJSON();
    REQUIRE(j2["links"].size() == 1);
    CHECK(j2["links"][0]["start"] == out);
    CHECK(j2["links"][0]["end"] == in);
}

TEST_CASE("ndsp Flowgraph setJSON keeps unknown nodes as missing nodes", "[dsp][flowgraph][missing_node]")
{
    Flowgraph fg;
    registerTestNodes(fg);
    addBlock(fg, "file_source_f");
    addBlock(fg, "file_sink_f");
    nlohmann::json j = fg.getJSON();
    j["nodes"][1]["int_id"] = "some_plugin_block";
    j["nodes"][1]["int_cfg"] = {{"opaque", {1, 2, 3}}};

    Flowgraph fg2;
    registerTestNodes(fg2);
    fg2.setJSON(j);
    nlohmann::json j2 = fg2.getJSON();
    REQUIRE(j2["nodes"].size() == 2);
    // The unknown node and its parameters survive a save
    CHECK(j2["nodes"][1]["int_id"] == "some_plugin_block");
    CHECK(j2["nodes"][1]["int_cfg"] == nlohmann::json{{"opaque", {1, 2, 3}}});
    CHECK(j2["nodes"][1]["io"] == j["nodes"][1]["io"]);

    // Nodes without int_id are dropped
    j["nodes"][0].erase("int_id");
    fg2.setJSON(j);
    CHECK(fg2.getJSON()["nodes"].size() == 2);
    CHECK(fg2.getJSON()["nodes"][0].is_null());
}

TEST_CASE("ndsp NodeMissing is errored and stores raw parameters", "[dsp][flowgraph][missing_node]")
{
    Flowgraph fg;
    NodeMissing n(&fg, 1, 0);
    CHECK(n.is_errored());
    CHECK(n.blk->d_id == "dummy");
    n.setP({{"a", 1}});
    CHECK(n.getP() == nlohmann::json{{"a", 1}});
    CHECK_FALSE(n.render());
}

// Bug: missing_node.h:23, DummyBlock resizes inputs twice (the second call
// should be outputs.resize), so a placeholder block never has outputs and
// gets max(ni, no) inputs instead.
TEST_CASE("ndsp DummyBlock has the requested inputs and outputs", "[dsp][flowgraph][missing_node][!shouldfail]")
{
    DummyBlock b(1, 2);
    CHECK(b.get_inputs().size() == 1);
    CHECK(b.get_outputs().size() == 2);
}

TEST_CASE("ndsp Flowgraph updateVars evaluates parameter expressions", "[dsp][flowgraph]")
{
    Flowgraph fg;
    registerTestNodes(fg);
    addBlock(fg, "multiply_constf");
    nlohmann::json j = fg.getJSON();
    j["vars"] = {{"a", 3.0}, {"b", 0.5}};
    j["nodes"][0]["vars"] = {{"const", "a * 2 + b"}};
    fg.setJSON(j);
    fg.updateVars();
    CHECK(fg.getJSON()["nodes"][0]["int_cfg"]["const"] == 6.5);

    // Changing a variable and updating again
    fg.variables["a"] = -1;
    fg.updateVars();
    CHECK(fg.getJSON()["nodes"][0]["int_cfg"]["const"] == -1.5);

    // Invalid expressions are logged and leave the value alone
    j = fg.getJSON();
    j["nodes"][0]["vars"] = {{"const", "a * * 2"}};
    fg.setJSON(j);
    fg.updateVars();
    CHECK(fg.getJSON()["nodes"][0]["int_cfg"]["const"] == -1.5);
}

TEST_CASE("ndsp Flowgraph runs a file to file chain", "[dsp][flowgraph][run]")
{
    ndsp_test::TempFile in_file, out_file;
    std::vector<float> data(4096);
    for (size_t i = 0; i < data.size(); i++)
        data[i] = (float)i - 2000.0f;
    ndsp_test::writeBytes(in_file.str(), data);

    auto fg = std::make_shared<Flowgraph>();
    registerTestNodes(*fg);
    addBlock(*fg, "file_source_f", {{"file", in_file.str()}, {"buffer_size", 1024}});
    addBlock(*fg, "multiply_constf", {{"const", -0.5}});
    addBlock(*fg, "file_sink_f", {{"file", out_file.str()}});
    nlohmann::json j = fg->getJSON();
    j["links"] = {link(0, ioIds(j["nodes"][0], true)[0], ioIds(j["nodes"][1], false)[0]), //
                  link(1, ioIds(j["nodes"][1], true)[0], ioIds(j["nodes"][2], false)[0])};
    fg->setJSON(j);

    CHECK_FALSE(fg->isRunning());
    REQUIRE(runFlowgraph(fg));
    CHECK_FALSE(fg->isRunning());

    auto out = ndsp_test::readBytes<float>(out_file.str());
    // FileSourceBlock pads the end with stale samples (see its bug test), check the prefix
    REQUIRE(out.size() >= data.size());
    size_t mismatches = 0, first = 0;
    for (size_t i = 0; i < data.size(); i++)
        if (!(out[i] == data[i] * -0.5f) && mismatches++ == 0)
            first = i;
    INFO("first mismatch at sample " << first);
    REQUIRE(mismatches == 0);

    // It can run again
    std::filesystem::remove(out_file.str());
    REQUIRE(runFlowgraph(fg));
    CHECK(ndsp_test::readBytes<float>(out_file.str()).size() >= data.size());
}

TEST_CASE("ndsp Flowgraph inserts a splitter for outputs with several links", "[dsp][flowgraph][run]")
{
    ndsp_test::TempFile in_file, out1, out2;
    auto data = ndsp_test::randomFloats(2048, 91);
    ndsp_test::writeBytes(in_file.str(), data);

    auto fg = std::make_shared<Flowgraph>();
    registerTestNodes(*fg);
    addBlock(*fg, "file_source_f", {{"file", in_file.str()}, {"buffer_size", 256}});
    addBlock(*fg, "file_sink_f", {{"file", out1.str()}});
    addBlock(*fg, "file_sink_f", {{"file", out2.str()}});
    nlohmann::json j = fg->getJSON();
    int src = ioIds(j["nodes"][0], true)[0];
    // Links can be drawn in either direction
    j["links"] = {link(0, src, ioIds(j["nodes"][1], false)[0]), link(1, ioIds(j["nodes"][2], false)[0], src)};
    fg->setJSON(j);
    REQUIRE(runFlowgraph(fg));

    for (auto *f : {&out1, &out2})
    {
        auto out = ndsp_test::readBytes<float>(f->str());
        REQUIRE(out.size() >= data.size());
        CHECK(std::vector<float>(out.begin(), out.begin() + data.size()) == data);
    }
}

TEST_CASE("ndsp Flowgraph refuses to run with unconnected IOs", "[dsp][flowgraph][run]")
{
    ndsp_test::TempFile out_file;
    auto fg = std::make_shared<Flowgraph>();
    registerTestNodes(*fg);

    // Unconnected input : the sink is never started, so its file is never created
    addBlock(*fg, "file_sink_f", {{"file", out_file.str()}});
    REQUIRE(runFlowgraph(fg));
    CHECK_FALSE(fg->isRunning());
    CHECK_FALSE(std::filesystem::exists(out_file.str()));

    // Unconnected output
    auto fg2 = std::make_shared<Flowgraph>();
    registerTestNodes(*fg2);
    addBlock(*fg2, "waveform_f");
    REQUIRE(runFlowgraph(fg2));
    CHECK_FALSE(fg2->isRunning());
}

TEST_CASE("ndsp Flowgraph does not run graphs with missing nodes", "[dsp][flowgraph][run]")
{
    ndsp_test::TempFile out_file;
    auto fg = std::make_shared<Flowgraph>();
    registerTestNodes(*fg);
    addBlock(*fg, "file_sink_f", {{"file", out_file.str()}});
    nlohmann::json j = fg->getJSON();
    j["nodes"][1] = j["nodes"][0];
    j["nodes"][1]["id"] = 1;
    j["nodes"][1]["int_id"] = "not_registered";
    j["nodes"][1]["io"][0]["id"] = 100;
    j["nodes"][1]["io"][0]["is_out"] = true;
    j["links"] = {link(0, 100, j["nodes"][0]["io"][0]["id"].get<int>())};
    fg->setJSON(j);
    REQUIRE(runFlowgraph(fg));
    CHECK_FALSE(std::filesystem::exists(out_file.str()));
}

TEST_CASE("ndsp Flowgraph ignores disabled nodes", "[dsp][flowgraph][run]")
{
    ndsp_test::TempFile in_file, out_file, unused;
    ndsp_test::writeBytes(in_file.str(), std::vector<float>(512, 3.0f));
    auto fg = std::make_shared<Flowgraph>();
    registerTestNodes(*fg);
    addBlock(*fg, "file_source_f", {{"file", in_file.str()}, {"buffer_size", 512}});
    addBlock(*fg, "file_sink_f", {{"file", out_file.str()}});
    addBlock(*fg, "file_sink_f", {{"file", unused.str()}}); // Unconnected, but disabled
    nlohmann::json j = fg->getJSON();
    j["links"] = {link(0, ioIds(j["nodes"][0], true)[0], ioIds(j["nodes"][1], false)[0])};
    j["nodes"][2]["disabled"] = true;
    fg->setJSON(j);
    REQUIRE(runFlowgraph(fg));
    CHECK(ndsp_test::readBytes<float>(out_file.str()).size() >= 512);
    CHECK_FALSE(std::filesystem::exists(unused.str()));
}

TEST_CASE("ndsp Flowgraph stop ends a graph driven by an endless source", "[dsp][flowgraph][run]")
{
    ndsp_test::TempFile out_file;
    auto fg = std::make_shared<Flowgraph>();
    registerTestNodes(*fg);
    addBlock(*fg, "waveform_f", {{"freq", 100.0}, {"samprate", 1000.0}});
    addBlock(*fg, "file_sink_f", {{"file", out_file.str()}});
    nlohmann::json j = fg->getJSON();
    j["links"] = {link(0, ioIds(j["nodes"][0], true)[0], ioIds(j["nodes"][1], false)[0])};
    fg->setJSON(j);

    bool saw_data = false;
    REQUIRE(runFlowgraph(fg,
                         [&]()
                         {
                             saw_data = ndsp_test::waitUntil(
                                 [&]()
                                 {
                                     std::error_code ec;
                                     return std::filesystem::file_size(out_file.str(), ec) >= 8192 * sizeof(float) && !ec;
                                 });
                             // Only stop once the flowgraph is really running
                             ndsp_test::waitUntil([&]() { return fg->isRunning(); });
                             fg->stop();
                         }));
    CHECK(saw_data);
    CHECK_FALSE(fg->isRunning());
    auto out = ndsp_test::readBytes<float>(out_file.str());
    REQUIRE(out.size() >= 8192);
    // 100 Hz at 1 kHz : period of 10 samples
    for (size_t i = 0; i < 100; i++)
        REQUIRE_THAT(out[i], WithinAbs(cos(2 * M_PI * 0.1 * i), 1e-3));
}

TEST_CASE("ndsp Flowgraph registry gets one entry per IO type of a block", "[dsp][flowgraph][register]")
{
    // The registry is keyed by d_id, a duplicate would silently hide a block
    Flowgraph fg;
    registerTestNodes(fg);
    size_t before = fg.node_internal_registry.size();
    registerNodeSimple<FileSourceBlock<complex_t>>(fg, "a");
    registerNodeSimple<FileSourceBlock<int16_t>>(fg, "b");
    registerNodeSimple<FileSourceBlock<int8_t>>(fg, "c");
    registerNodeSimple<FileSourceBlock<uint8_t>>(fg, "d");
    registerNodeSimple<FileSinkBlock<complex_t>>(fg, "e");
    registerNodeSimple<FileSinkBlock<int16_t>>(fg, "f");
    registerNodeSimple<FileSinkBlock<int8_t>>(fg, "g");
    registerNodeSimple<FileSinkBlock<uint8_t>>(fg, "h");
    registerNodeSimple<WaveformBlock<complex_t>>(fg, "i");
    registerNodeSimple<MultiplyConstBlock<complex_t>>(fg, "j");
    registerNodeSimple<AddBlock<complex_t>>(fg, "k");
    CHECK(fg.node_internal_registry.size() == before + 11);
}
