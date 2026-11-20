#include "core/exception.h"
#include "pipeline/module.h"
#include "pipeline_test_helpers.h"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>

using namespace pipeline_test_helpers;
namespace pl = satdump::pipeline;

namespace
{
    // Minimal module relying on all the ProcessingModule defaults
    class DefaultsModule : public pl::ProcessingModule
    {
    public:
        DefaultsModule(std::string in, std::string hint, nlohmann::json params) : ProcessingModule(in, hint, params) {}
        void process() {}
        void drawUI(bool) {}
        std::string getIDM() { return "unit_test_defaults"; }

        // Expose protected state for checks
        const std::string &inputFile() { return d_input_file; }
        const std::string &hint() { return d_output_file_hint; }
        nlohmann::json &params() { return d_parameters; }
        bool streaming() { return d_is_streaming_input; }
    };

    class StreamingModule : public DefaultsModule
    {
    public:
        using DefaultsModule::DefaultsModule;
        std::vector<pl::ModuleDataType> getInputTypes() { return {pl::DATA_FILE, pl::DATA_STREAM, pl::DATA_DSP_STREAM}; }
        std::vector<pl::ModuleDataType> getOutputTypes() { return {pl::DATA_STREAM}; }
    };

    bool hasId(const std::string &id)
    {
        return std::any_of(pl::modules_registry.begin(), pl::modules_registry.end(), [&](const pl::ModuleEntry &e) { return e.id == id; });
    }
} // namespace

TEST_CASE("ProcessingModule constructor stores arguments and defaults", "[pipeline][module]")
{
    nlohmann::json p = {{"a", 1}};
    DefaultsModule m("/in/file.bin", "/out/hint", p);

    CHECK(m.inputFile() == "/in/file.bin");
    CHECK(m.hint() == "/out/hint");
    CHECK(m.params() == p);
    CHECK_FALSE(m.input_active.load());
    CHECK_FALSE(m.streaming());
    CHECK(m.getOutput() == "");
    CHECK(m.getModuleStats().is_null());
    CHECK(m.input_fifo == nullptr);
    CHECK(m.output_fifo == nullptr);
    CHECK(m.input_stream == nullptr);
    CHECK(m.output_stream == nullptr);

    // Defaults only support files
    CHECK(m.getInputTypes() == std::vector<pl::ModuleDataType>{pl::DATA_FILE});
    CHECK(m.getOutputTypes() == std::vector<pl::ModuleDataType>{pl::DATA_FILE});

    // Base init/stop are no-ops
    m.init();
    m.stop();
}

TEST_CASE("ProcessingModule::setInputType/setOutputType validate against supported types", "[pipeline][module]")
{
    DefaultsModule m("in", "out", {});

    m.setInputType(pl::DATA_FILE);
    m.setOutputType(pl::DATA_FILE);
    CHECK(m.getInputType() == pl::DATA_FILE);
    CHECK(m.getOutputType() == pl::DATA_FILE);
    CHECK_FALSE(m.streaming());

    CHECK_THROWS_AS(m.setInputType(pl::DATA_STREAM), satdump::satdump_exception_t);
    CHECK_THROWS_AS(m.setOutputType(pl::DATA_DSP_STREAM), satdump::satdump_exception_t);

    StreamingModule s("in", "out", {});
    s.setInputType(pl::DATA_STREAM);
    CHECK(s.getInputType() == pl::DATA_STREAM);
    CHECK(s.streaming());
    s.setInputType(pl::DATA_DSP_STREAM);
    CHECK(s.streaming());
    s.setInputType(pl::DATA_FILE);
    CHECK_FALSE(s.streaming());

    s.setOutputType(pl::DATA_STREAM);
    CHECK(s.getOutputType() == pl::DATA_STREAM);
    CHECK_THROWS_AS(s.setOutputType(pl::DATA_FILE), satdump::satdump_exception_t);
}

TEST_CASE("ProcessingModule::setInputType error message names the module", "[pipeline][module]")
{
    DefaultsModule m("in", "out", {});
    try
    {
        m.setInputType(pl::DATA_NDSP_STREAM);
        FAIL("expected an exception");
    }
    catch (satdump::satdump_exception_t &e)
    {
        CHECK(std::string(e.what()).find("unit_test_defaults") != std::string::npos);
    }
}

TEST_CASE("pipeline::moduleExists and getModuleInstance look up the registry", "[pipeline][module][registry]")
{
    ModuleRegistryGuard reg;
    CallsGuard calls;

    CHECK_FALSE(pl::moduleExists("unit_test_mod_a"));
    CHECK_THROWS_AS(pl::getModuleInstance("unit_test_mod_a", "i", "h", {}), satdump::satdump_exception_t);

    pl::modules_registry.push_back(recordingEntry("unit_test_mod_a"));
    pl::modules_registry.push_back(recordingEntry("unit_test_mod_b"));

    CHECK(pl::moduleExists("unit_test_mod_a"));
    CHECK(pl::moduleExists("unit_test_mod_b"));
    CHECK_FALSE(pl::moduleExists("unit_test_mod"));
    CHECK_FALSE(pl::moduleExists(""));

    auto m = pl::getModuleInstance("unit_test_mod_b", "/x/in.bin", "/x/out", {{"k", "v"}});
    REQUIRE(m != nullptr);
    CHECK(m->getIDM() == "unit_test_mod_b");

    auto c = callsOf("construct");
    REQUIRE(c.size() == 1);
    CHECK(c[0].id == "unit_test_mod_b");
    CHECK(c[0].input == "/x/in.bin");
    CHECK(c[0].hint == "/x/out");
    CHECK(c[0].params == nlohmann::json{{"k", "v"}});
}

TEST_CASE("pipeline::getModuleInstance returns the first matching registry entry", "[pipeline][module][registry]")
{
    ModuleRegistryGuard reg;
    CallsGuard calls;

    pl::modules_registry.push_back(recordingEntry("unit_test_dup"));
    pl::modules_registry.push_back({"unit_test_dup", {}, [](std::string, std::string, nlohmann::json) -> std::shared_ptr<pl::ProcessingModule> { return nullptr; }});

    auto m = pl::getModuleInstance("unit_test_dup", "", "", {});
    CHECK(m != nullptr);
}

TEST_CASE("pipeline::registerModules registers the core modules and plugin modules", "[pipeline][module][registry]")
{
    ModuleRegistryGuard reg;
    EventBusGuard bus;

    // A plugin hooks the registration event to add its own modules
    satdump::eventBus->register_handler<pl::RegisterModulesEvent>([](pl::RegisterModulesEvent evt) { evt.modules_registry.push_back(recordingEntry("unit_test_plugin_module")); });

    pl::registerModules();

    const std::vector<std::string> core = {"fsk_demod",
                                           "pm_demod",
                                           "psk_demod",
                                           "sdpsk_demod",
                                           "xfsk_burst_demod",
                                           "network_server",
                                           "network_client",
                                           "xrit_goesrecv_publisher",
                                           "s2_udp_cadu_extractor",
                                           "ccsds_conv_concat_decoder",
                                           "ccsds_simple_psk_decoder",
                                           "ccsds_ldpc_decoder",
                                           "ccsds_turbo_decoder",
                                           "products_processor",
                                           "soft2hard",
                                           "ndsp_demod"};

    for (auto &id : core)
    {
        INFO(id);
        CHECK(hasId(id));
    }
    CHECK(hasId("unit_test_plugin_module"));
    CHECK(pl::modules_registry.size() == core.size() + 1);

    // Plugins are registered after all core modules
    CHECK(pl::modules_registry.back().id == "unit_test_plugin_module");

    // IDs are unique
    std::vector<std::string> ids;
    for (auto &e : pl::modules_registry)
        ids.push_back(e.id);
    std::sort(ids.begin(), ids.end());
    CHECK(std::adjacent_find(ids.begin(), ids.end()) == ids.end());

    // Every entry is instantiable through a factory
    for (auto &e : pl::modules_registry)
    {
        INFO(e.id);
        CHECK(e.inst != nullptr);
    }
}

TEST_CASE("pipeline::registerModules core instances report their registry ID", "[pipeline][module][registry]")
{
    ModuleRegistryGuard reg;
    EventBusGuard bus;
    TempDir tmp("pipe_regid");

    pl::registerModules();

    // Only modules whose constructors need nothing but trivially-satisfied parameters
    auto soft2hard = pl::getModuleInstance("soft2hard", tmp.file("in"), tmp.file("out"), {});
    CHECK(soft2hard->getIDM() == "soft2hard");

    auto pp = pl::getModuleInstance("products_processor", tmp.file("dataset.json"), tmp.file("out"), {});
    CHECK(pp->getIDM() == "products_processor");
    CHECK(pp->getInputTypes() == std::vector<pl::ModuleDataType>{pl::DATA_FILE});

    auto psk = pl::getModuleInstance("ccsds_simple_psk_decoder", tmp.file("in"), tmp.file("out"), {{"constellation", "bpsk"}, {"cadu_size", 8192}, {"rs_i", 0}});
    CHECK(psk->getIDM() == "ccsds_simple_psk_decoder");

    auto s2 = pl::getModuleInstance("s2_udp_cadu_extractor", tmp.file("in"), tmp.file("out"), {{"pid", 100}, {"bb_size", 38688}});
    CHECK(s2->getIDM() == "s2_udp_cadu_extractor");
}
