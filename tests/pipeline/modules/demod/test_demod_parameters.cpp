#include "pipeline/modules/demod/module_demod_base.h"
#include "../../pipeline_test_helpers.h"
#include "core/exception.h"
#include <catch2/catch_test_macros.hpp>

using namespace pipeline_test_helpers;
namespace pl = satdump::pipeline;

// Only constructor-level parameter validation is covered: actually running a demodulator needs
// a realistic baseband signal and a full DSP chain.

TEST_CASE("Demodulator modules require a samplerate parameter", "[pipeline][modules][demod]")
{
    ModuleRegistryGuard reg;
    EventBusGuard bus;
    MainConfigGuard cfg;
    pl::registerModules();

    for (std::string id : {"psk_demod", "fsk_demod", "pm_demod", "sdpsk_demod", "xfsk_burst_demod"})
    {
        INFO(id);
        CHECK_THROWS_AS(pl::getModuleInstance(id, "in.cf32", "out", {{"symbolrate", 1000}}), satdump::satdump_exception_t);
    }
}

TEST_CASE("BaseDemodModule::getParams documents defaults for common demodulator parameters", "[pipeline][modules][demod]")
{
    auto p = pl::demod::BaseDemodModule::getParams();
    CHECK(p["samplerate"] == 6e6);
    CHECK(p["symbolrate"] == 2e6);
    CHECK(p["dc_block"] == false);
    CHECK(p["iq_swap"] == false);
    CHECK(p["enable_doppler"] == false);
    CHECK(p["baseband_format"] == "cf32");
}

TEST_CASE("PSK demodulator validates its constellation parameter", "[pipeline][modules][demod]")
{
    ModuleRegistryGuard reg;
    EventBusGuard bus;
    MainConfigGuard cfg;
    pl::registerModules();
    satdump::satdump_cfg.main_cfg["user_interface"]["show_waterfall_demod_fft"]["value"] = false;

    nlohmann::json params = {{"samplerate", 48000}, {"symbolrate", 1200}, {"rrc_alpha", 0.5}, {"pll_bw", 0.01}};

    SECTION("missing constellation")
    {
        CHECK_THROWS_AS(pl::getModuleInstance("psk_demod", "in.cf32", "out", params), satdump::satdump_exception_t);
    }

    SECTION("valid parameters")
    {
        params["constellation"] = "bpsk";
        std::shared_ptr<pl::ProcessingModule> m;
        REQUIRE_NOTHROW(m = pl::getModuleInstance("psk_demod", "in.cf32", "out", params));
        CHECK(m->getIDM() == "psk_demod");
        auto in = m->getInputTypes();
        CHECK(std::find(in.begin(), in.end(), pl::DATA_FILE) != in.end());
        CHECK(std::find(in.begin(), in.end(), pl::DATA_DSP_STREAM) != in.end());
    }
}
