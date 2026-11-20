#include "common/dsp_source_sink/dsp_sample_sink.h"
#include <catch2/catch_test_macros.hpp>

namespace
{
    const std::string FAKE_SINK = "unit_test_fake_sink";

    struct FakeSinkRegistry
    {
        std::vector<dsp::SinkDescriptor> created;

        FakeSinkRegistry()
        {
            dsp::dsp_sinks_registry[FAKE_SINK] = {
                [this](dsp::SinkDescriptor d) -> std::shared_ptr<dsp::DSPSampleSink>
                {
                    created.push_back(d);
                    return nullptr;
                },
                []()
                { return std::vector<dsp::SinkDescriptor>{{FAKE_SINK, "Fake Sink", "sink_0"}}; }};
        }

        ~FakeSinkRegistry() { dsp::dsp_sinks_registry.erase(FAKE_SINK); }
    };
} // namespace

TEST_CASE("dsp::getAllAvailableSinks lists registered sinks", "[common][dsp_source_sink][dsp_sample_sink]")
{
    FakeSinkRegistry reg;
    auto all = dsp::getAllAvailableSinks();

    int n = 0;
    for (auto &s : all)
        if (s.sink_type == FAKE_SINK)
        {
            n++;
            CHECK(s.unique_id == "sink_0");
        }
    CHECK(n == 1);
}

TEST_CASE("dsp::getSinkFromDescriptor dispatches on sink type", "[common][dsp_source_sink][dsp_sample_sink]")
{
    FakeSinkRegistry reg;

    dsp::getSinkFromDescriptor({FAKE_SINK, "Fake Sink", "sink_0"});
    REQUIRE(reg.created.size() == 1);
    CHECK(reg.created[0].unique_id == "sink_0");

    CHECK_THROWS_AS(dsp::getSinkFromDescriptor({"unit_test_no_such_sink", "x", "y"}), satdump::satdump_exception_t);
}
