#include "cli_test_helpers.h"
#include "legacy/sdr_probe.h"
#include <catch2/catch_test_macros.hpp>

using namespace cli_test;

namespace
{
    const std::string FAKE_SOURCE = "unit_test_cli_source";
    const std::string FAKE_SINK = "unit_test_cli_sink";
} // namespace

TEST_CASE("sdr_probe lists the file source when no plugin is loaded", "[cli][legacy][sdr_probe]")
{
    EventBusGuard bus;
    DeviceRegistryGuard devices;
    dsp::dsp_sources_registry.clear();
    dsp::dsp_sinks_registry.clear();
    LogCapture log;

    sdr_probe();

    INFO(log.dump());
    CHECK(log.contains("Found devices (sources) :", slog::LOG_INFO));
    CHECK(log.contains("    - type : file", slog::LOG_DEBUG));
    CHECK(log.contains("    - id : 0", slog::LOG_DEBUG));
    CHECK(log.contains("Found devices (sinks) :", slog::LOG_INFO));

    // sdr_probe registers the built-in devices itself
    CHECK(dsp::dsp_sources_registry.count("file") == 1);
    CHECK(dsp::dsp_sinks_registry.empty());
}

TEST_CASE("sdr_probe lists the sources and sinks registered by plugins", "[cli][legacy][sdr_probe]")
{
    EventBusGuard bus;
    DeviceRegistryGuard devices;
    dsp::dsp_sources_registry.clear();
    dsp::dsp_sinks_registry.clear();

    // What a SDR plugin does when sdr_probe asks for sources and sinks
    satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(
        [](const dsp::RegisterDSPSampleSourcesEvent &evt)
        {
            evt.dsp_sources_registry[FAKE_SOURCE] = {[](dsp::SourceDescriptor) -> std::shared_ptr<dsp::DSPSampleSource> { return nullptr; },
                                                     []() { return std::vector<dsp::SourceDescriptor>{{FAKE_SOURCE, "Unit RX One", "rx_1"}, {FAKE_SOURCE, "Unit RX Two", "rx_2"}}; }};
        });
    satdump::eventBus->register_handler<dsp::RegisterDSPSampleSinksEvent>(
        [](const dsp::RegisterDSPSampleSinksEvent &evt)
        {
            evt.dsp_sinks_registry[FAKE_SINK] = {[](dsp::SinkDescriptor) -> std::shared_ptr<dsp::DSPSampleSink> { return nullptr; },
                                                 []() { return std::vector<dsp::SinkDescriptor>{{FAKE_SINK, "Unit TX", "tx_1"}}; }};
        });

    LogCapture log;
    sdr_probe();

    INFO(log.dump());
    CHECK(log.contains("- Unit RX One", slog::LOG_INFO));
    CHECK(log.contains("- Unit RX Two", slog::LOG_INFO));
    CHECK(log.contains("    - type : " + FAKE_SOURCE, slog::LOG_DEBUG));
    CHECK(log.contains("    - id : rx_1", slog::LOG_DEBUG));
    CHECK(log.contains("    - id : rx_2", slog::LOG_DEBUG));
    CHECK(log.contains("    - type : file", slog::LOG_DEBUG));

    CHECK(log.contains("- Unit TX", slog::LOG_INFO));
    CHECK(log.contains("    - type : " + FAKE_SINK, slog::LOG_DEBUG));
    CHECK(log.contains("    - id : tx_1", slog::LOG_DEBUG));
}
