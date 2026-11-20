#include "core/exception.h"
#include "libs/ctpl/ctpl_stl.h"
#include "pipeline/live_pipeline.h"
#include "pipeline/pipeline_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <map>

using namespace pipeline_test;
namespace pl = satdump::pipeline;

namespace
{
    struct LiveLog
    {
        std::mutex mtx;
        std::vector<std::string> constructed;
        std::map<std::string, nlohmann::json> params;
        std::map<std::string, std::string> hints;
        std::map<std::string, std::string> inputs;
    };

    LiveLog &liveLog()
    {
        static LiveLog l;
        return l;
    }

    void resetLiveLog()
    {
        std::lock_guard<std::mutex> l(liveLog().mtx);
        liveLog().constructed.clear();
        liveLog().params.clear();
        liveLog().hints.clear();
        liveLog().inputs.clear();
    }

    class LiveModuleBase : public pl::ProcessingModule
    {
    protected:
        std::string id;

    public:
        LiveModuleBase(std::string id, std::string in, std::string hint, nlohmann::json params) : ProcessingModule(in, hint, params), id(id)
        {
            std::lock_guard<std::mutex> l(liveLog().mtx);
            liveLog().constructed.push_back(id);
            liveLog().params[id] = params;
            liveLog().hints[id] = hint;
            liveLog().inputs[id] = in;
        }
        void drawUI(bool) {}
        std::string getIDM() { return id; }
    };

    // Takes complex samples, outputs the real part of each one as a byte
    class LiveDemod : public LiveModuleBase
    {
    public:
        using LiveModuleBase::LiveModuleBase;
        std::vector<pl::ModuleDataType> getInputTypes() { return {pl::DATA_DSP_STREAM}; }
        std::vector<pl::ModuleDataType> getOutputTypes() { return {pl::DATA_STREAM, pl::DATA_FILE}; }

        void process()
        {
            while (true)
            {
                int n = input_stream->read();
                if (n <= 0)
                    break;
                std::vector<uint8_t> bytes(n);
                for (int i = 0; i < n; i++)
                    bytes[i] = (uint8_t)input_stream->readBuf[i].real;
                input_stream->flush();
                output_fifo->write(bytes.data(), n);
            }
        }
    };

    // Middle module: adds 1 to every byte
    class LiveIncrement : public LiveModuleBase
    {
    public:
        using LiveModuleBase::LiveModuleBase;
        std::vector<pl::ModuleDataType> getInputTypes() { return {pl::DATA_STREAM}; }
        std::vector<pl::ModuleDataType> getOutputTypes() { return {pl::DATA_STREAM}; }

        void process()
        {
            uint8_t b;
            while (input_fifo->read(&b, 1) == 1)
            {
                b++;
                output_fifo->write(&b, 1);
            }
        }
    };

    // Final module: writes the stream to <hint>.live
    class LiveSink : public LiveModuleBase
    {
    public:
        std::atomic<int> count{0};
        std::vector<uint8_t> received;
        std::mutex rx_mtx;

        using LiveModuleBase::LiveModuleBase;
        std::vector<pl::ModuleDataType> getInputTypes() { return {pl::DATA_STREAM}; }
        std::vector<pl::ModuleDataType> getOutputTypes() { return {pl::DATA_FILE}; }

        void init() { d_output_file = d_output_file_hint + ".live"; }

        void process()
        {
            uint8_t b;
            while (input_fifo->read(&b, 1) == 1)
            {
                {
                    std::lock_guard<std::mutex> l(rx_mtx);
                    received.push_back(b);
                }
                count++;
            }
        }

        nlohmann::json getModuleStats() { return {{"count", count.load()}}; }
    };

    // Stand-in for network_client: produces bytes without any input.
    // It provides its own input FIFO so that LivePipeline::stop() can stop it (see note below).
    class LiveFakeClient : public LiveModuleBase
    {
    public:
        using LiveModuleBase::LiveModuleBase;
        std::vector<pl::ModuleDataType> getInputTypes() { return {pl::DATA_STREAM}; }
        std::vector<pl::ModuleDataType> getOutputTypes() { return {pl::DATA_STREAM}; }

        void init()
        {
            if (!input_fifo)
                input_fifo = std::make_shared<dsp::RingBuffer<uint8_t>>(1000);
        }

        void process()
        {
            std::vector<uint8_t> data = {10, 20, 30, 40};
            output_fifo->write(data.data(), data.size());
            while (input_active)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    };

    template <typename T>
    pl::ModuleEntry liveEntry(const std::string &id, std::shared_ptr<T> *keep = nullptr)
    {
        return pl::ModuleEntry{id, nlohmann::json::object(), [id, keep](std::string in, std::string hint, nlohmann::json params) -> std::shared_ptr<pl::ProcessingModule>
                               {
                                   auto m = std::make_shared<T>(id, in, hint, params);
                                   if (keep)
                                       *keep = m;
                                   return m;
                               }};
    }

    struct LiveEnv
    {
        ModuleRegistryGuard reg;
        TempDir dir;
        LiveEnv() : dir("pipe_live") { resetLiveLog(); }
        ~LiveEnv() { resetLiveLog(); }
    };

    pl::Pipeline makeLivePipeline()
    {
        pl::Pipeline p;
        p.id = "unit_live";
        p.name = "Unit Live";
        p.live = true;
        p.steps = {{"baseband", "", {}, ""}, {"soft", "unit_live_demod", {{"symbolrate", 1000}, {"shared", "module"}}, ""}, {"mid", "unit_live_inc", {}, ""}, {"frames", "unit_live_sink", {}, ""}};
        return p;
    }

    void pushSamples(dsp::stream<complex_t> &stream, const std::vector<uint8_t> &values)
    {
        for (size_t i = 0; i < values.size(); i++)
            stream.writeBuf[i] = complex_t((float)values[i], 0);
        stream.swap(values.size());
    }
} // namespace

TEST_CASE("LivePipeline::start wires demod, middle and sink modules through FIFOs", "[pipeline][live_pipeline]")
{
    LiveEnv env;
    std::shared_ptr<LiveSink> sink;
    pl::modules_registry.push_back(liveEntry<LiveDemod>("unit_live_demod"));
    pl::modules_registry.push_back(liveEntry<LiveIncrement>("unit_live_inc"));
    pl::modules_registry.push_back(liveEntry<LiveSink>("unit_live_sink", &sink));

    auto p = makeLivePipeline();
    p.live_cfg.normal_live = {1, 2, 3};

    ctpl::thread_pool tp(4);
    auto stream = std::make_shared<dsp::stream<complex_t>>();

    {
        pl::LivePipeline live(p, {{"shared", "user"}, {"extra", 5}}, env.dir.str());
        live.start(stream, tp);

        REQUIRE(live.modules.size() == 3);
        REQUIRE(sink != nullptr);
        CHECK(live.modules[0]->getInputType() == pl::DATA_DSP_STREAM);
        CHECK(live.modules[0]->getOutputType() == pl::DATA_STREAM);
        CHECK(live.modules[1]->getInputType() == pl::DATA_STREAM);
        CHECK(live.modules[1]->getOutputType() == pl::DATA_STREAM);
        CHECK(live.modules[2]->getInputType() == pl::DATA_STREAM);
        CHECK(live.modules[2]->getOutputType() == pl::DATA_FILE);
        CHECK(live.modules[1]->input_fifo == live.modules[0]->output_fifo);
        CHECK(live.modules[2]->input_fifo == live.modules[1]->output_fifo);
        CHECK(live.modules[0]->input_stream == stream);

        pushSamples(*stream, {1, 2, 3, 4});
        pushSamples(*stream, {5, 6});
        pushSamples(*stream, {100});

        REQUIRE(test_helpers::waitUntil([&]() { return sink->count.load() >= 7; }, std::chrono::seconds(30)));

        auto stats = live.getModulesStats();
        CHECK(stats["unit_live_sink"]["count"] == 7);
        CHECK(stats.contains("unit_live_demod"));
        CHECK(stats.contains("unit_live_inc"));

        live.stop();

        CHECK(live.getOutputFile() == env.dir.str() + "/unit_live.live");
    }

    {
        std::lock_guard<std::mutex> l(sink->rx_mtx);
        CHECK(sink->received == std::vector<uint8_t>{2, 3, 4, 5, 6, 7, 101});
    }

    std::lock_guard<std::mutex> l(liveLog().mtx);
    CHECK(liveLog().constructed == std::vector<std::string>{"unit_live_demod", "unit_live_inc", "unit_live_sink"});
    // Live modules get no input file, and user parameters override step parameters
    CHECK(liveLog().inputs["unit_live_demod"] == "");
    CHECK(liveLog().hints["unit_live_demod"] == env.dir.str() + "/unit_live");
    CHECK(liveLog().params["unit_live_demod"]["symbolrate"] == 1000);
    CHECK(liveLog().params["unit_live_demod"]["shared"] == "user");
    CHECK(liveLog().params["unit_live_sink"]["extra"] == 5);
}

TEST_CASE("LivePipeline::start with a single module outputs to a file", "[pipeline][live_pipeline]")
{
    LiveEnv env;
    pl::modules_registry.push_back(liveEntry<LiveDemod>("unit_live_demod"));

    auto p = makeLivePipeline();
    p.live_cfg.normal_live = {1};

    ctpl::thread_pool tp(2);
    auto stream = std::make_shared<dsp::stream<complex_t>>();
    pl::LivePipeline live(p, {}, env.dir.str());
    live.start(stream, tp);

    REQUIRE(live.modules.size() == 1);
    CHECK(live.modules[0]->getInputType() == pl::DATA_DSP_STREAM);
    CHECK(live.modules[0]->getOutputType() == pl::DATA_FILE);

    live.stop();
}

TEST_CASE("LivePipeline::start in server mode appends network_server with the packet size", "[pipeline][live_pipeline]")
{
    LiveEnv env;
    std::shared_ptr<LiveSink> server;
    pl::modules_registry.push_back(liveEntry<LiveDemod>("unit_live_demod"));
    pl::modules_registry.push_back(liveEntry<LiveSink>("network_server", &server));

    auto p = makeLivePipeline();
    p.live_cfg.normal_live = {1, 2, 3};
    p.live_cfg.server_live = {1};
    p.live_cfg.pkt_size = 1234;

    ctpl::thread_pool tp(2);
    auto stream = std::make_shared<dsp::stream<complex_t>>();
    pl::LivePipeline live(p, {{"user", true}}, env.dir.str());
    live.start(stream, tp, true);

    REQUIRE(live.modules.size() == 2);
    CHECK(live.modules[0]->getIDM() == "unit_live_demod");
    CHECK(live.modules[1]->getIDM() == "network_server");

    pushSamples(*stream, {9, 8});
    REQUIRE(test_helpers::waitUntil([&]() { return server->count.load() >= 2; }, std::chrono::seconds(30)));
    live.stop();

    std::lock_guard<std::mutex> l(liveLog().mtx);
    CHECK(liveLog().params["network_server"]["pkt_size"] == 1234);
    CHECK(liveLog().params["network_server"]["user"] == true);
    CHECK(liveLog().hints["network_server"] == env.dir.str() + "/unit_live");
}

// Note: LivePipeline::stop() (live_pipeline.cpp:174) dereferences input_fifo of the first module.
// In client mode the first module is network_client, which is never given an input FIFO, so the
// real module would crash on stop. The fake client creates its own FIFO to keep the test safe.
TEST_CASE("LivePipeline::start_client prepends network_client", "[pipeline][live_pipeline]")
{
    LiveEnv env;
    std::shared_ptr<LiveSink> sink;
    pl::modules_registry.push_back(liveEntry<LiveFakeClient>("network_client"));
    pl::modules_registry.push_back(liveEntry<LiveSink>("unit_live_sink", &sink));

    auto p = makeLivePipeline();
    p.live_cfg.normal_live = {1, 2, 3};
    p.live_cfg.client_live = {3};
    p.live_cfg.pkt_size = 4;

    ctpl::thread_pool tp(2);
    pl::LivePipeline live(p, {}, env.dir.str());
    live.start_client(tp);

    REQUIRE(live.modules.size() == 2);
    CHECK(live.modules[0]->getIDM() == "network_client");
    CHECK(live.modules[0]->getOutputType() == pl::DATA_STREAM);
    CHECK(live.modules[1]->getIDM() == "unit_live_sink");
    CHECK(live.modules[1]->input_fifo == live.modules[0]->output_fifo);

    REQUIRE(test_helpers::waitUntil([&]() { return sink->count.load() >= 4; }, std::chrono::seconds(30)));
    live.stop();

    CHECK(live.getOutputFile() == env.dir.str() + "/unit_live.live");
    {
        std::lock_guard<std::mutex> l(sink->rx_mtx);
        CHECK(sink->received == std::vector<uint8_t>{10, 20, 30, 40});
    }
    std::lock_guard<std::mutex> l(liveLog().mtx);
    CHECK(liveLog().params["network_client"]["pkt_size"] == 4);
}

TEST_CASE("LivePipeline rejects unsupported modes and invalid steps", "[pipeline][live_pipeline]")
{
    LiveEnv env;
    pl::modules_registry.push_back(liveEntry<LiveDemod>("unit_live_demod"));

    auto p = makeLivePipeline();
    p.live_cfg.normal_live = {1};

    ctpl::thread_pool tp(1);
    auto stream = std::make_shared<dsp::stream<complex_t>>();

    SECTION("server mode without server configuration")
    {
        pl::LivePipeline live(p, {}, env.dir.str());
        CHECK_THROWS_AS(live.start(stream, tp, true), satdump::satdump_exception_t);
        CHECK(live.modules.empty());
    }

    SECTION("client mode without client configuration")
    {
        pl::LivePipeline live(p, {}, env.dir.str());
        CHECK_THROWS_AS(live.start_client(tp), satdump::satdump_exception_t);
        CHECK(live.modules.empty());
    }

    SECTION("step index out of range")
    {
        p.live_cfg.normal_live = {7};
        pl::LivePipeline live(p, {}, env.dir.str());
        CHECK_THROWS_AS(live.start(stream, tp), satdump::satdump_exception_t);
        CHECK(live.modules.empty());
    }

    SECTION("step without a registered module")
    {
        p.live_cfg.normal_live = {2};
        pl::LivePipeline live(p, {}, env.dir.str());
        CHECK_THROWS_AS(live.start(stream, tp), satdump::satdump_exception_t);
    }
}
