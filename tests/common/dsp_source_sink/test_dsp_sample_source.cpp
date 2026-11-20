#include "common/dsp_source_sink/dsp_sample_source.h"
#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

namespace
{
    const std::string FAKE_TYPE = "unit_test_fake_source";
    const std::string THROWING_TYPE = "unit_test_throwing_source";

    // Registers fake entries in the global registry and removes them afterwards
    struct FakeRegistry
    {
        std::vector<dsp::SourceDescriptor> created;

        FakeRegistry()
        {
            dsp::dsp_sources_registry[FAKE_TYPE] = {
                [this](dsp::SourceDescriptor d) -> std::shared_ptr<dsp::DSPSampleSource>
                {
                    created.push_back(d);
                    return nullptr;
                },
                []()
                {
                    return std::vector<dsp::SourceDescriptor>{{FAKE_TYPE, "Fake A", "fake_a"}, {FAKE_TYPE, "Fake B", "fake_b"}};
                }};
            dsp::dsp_sources_registry[THROWING_TYPE] = {
                [](dsp::SourceDescriptor) -> std::shared_ptr<dsp::DSPSampleSource> { return nullptr; },
                []() -> std::vector<dsp::SourceDescriptor> { throw std::runtime_error("device enumeration failed"); }};
        }

        ~FakeRegistry()
        {
            dsp::dsp_sources_registry.erase(FAKE_TYPE);
            dsp::dsp_sources_registry.erase(THROWING_TYPE);
        }
    };

    int countType(const std::vector<dsp::SourceDescriptor> &v, const std::string &type)
    {
        int n = 0;
        for (auto &d : v)
            n += d.source_type == type;
        return n;
    }
} // namespace

TEST_CASE("dsp::getAllAvailableSources lists registered sources and survives throwing ones", "[common][dsp_source_sink][dsp_sample_source]")
{
    FakeRegistry reg;

    auto all = dsp::getAllAvailableSources();
    CHECK(countType(all, FAKE_TYPE) == 2);
    CHECK(countType(all, THROWING_TYPE) == 0);

    auto remote = dsp::getAllAvailableSources(true);
    CHECK(countType(remote, FAKE_TYPE) == 2);
}

TEST_CASE("dsp::getSourceFromDescriptor dispatches on source type", "[common][dsp_source_sink][dsp_sample_source]")
{
    FakeRegistry reg;

    dsp::SourceDescriptor d{FAKE_TYPE, "Fake A", "fake_a"};
    dsp::getSourceFromDescriptor(d);
    REQUIRE(reg.created.size() == 1);
    CHECK(reg.created[0].unique_id == "fake_a");
    CHECK(reg.created[0].name == "Fake A");

    dsp::SourceDescriptor unknown{"unit_test_no_such_type", "Nothing", "none"};
    CHECK_THROWS_AS(dsp::getSourceFromDescriptor(unknown), satdump::satdump_exception_t);
}

TEST_CASE("dsp::SourceDescriptor serialises to JSON", "[common][dsp_source_sink][dsp_sample_source]")
{
    dsp::SourceDescriptor d{"rtlsdr", "RTL-SDR #0", "0001"};
    nlohmann::json j = d;
    CHECK(j["source_type"] == "rtlsdr");
    CHECK(j["name"] == "RTL-SDR #0");
    CHECK(j["unique_id"] == "0001");
    // remote_ok is not part of the serialised form
    CHECK_FALSE(j.contains("remote_ok"));

    dsp::SourceDescriptor back = j.get<dsp::SourceDescriptor>();
    CHECK(back.source_type == d.source_type);
    CHECK(back.name == d.name);
    CHECK(back.unique_id == d.unique_id);
}
