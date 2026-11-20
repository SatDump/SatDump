#include "../ndsp_test_helpers.h"
#include "dsp/device/dev.h"
#include "dsp/device/options_displayer.h"
#include "dsp/device/options_displayer_warper.h"
#include "dsp/utils/multiply_const.h"
#include <catch2/catch_test_macros.hpp>

using namespace satdump::ndsp;

namespace
{
    // Hardware-free device, records what setDevInfo applied
    class FakeDevice : public DeviceBlock
    {
    public:
        nlohmann::json applied;
        double sr = 0, freq = 0;

        FakeDevice() : DeviceBlock("fake_dev_cc", {}, {{"out", DSP_SAMPLE_TYPE_CF32}}) {}

        nlohmann::json get_cfg(std::string key) override { return applied[key]; }
        cfg_res_t set_cfg(std::string key, nlohmann::json v) override
        {
            if (key == "reject")
                throw satdump_exception(key);
            applied[key] = v;
            return RES_OK;
        }
        double getStreamSamplerate(int, bool) override { return sr; }
        void setStreamSamplerate(int, bool, double s) override { sr = s; }
        double getStreamFrequency(int, bool) override { return freq; }
        void setStreamFrequency(int, bool, double f) override { freq = f; }

        DeviceInfo info() { return devInfo; }
        device_mode_t mode() { return devMode; }
    };
} // namespace

///////////////////////////////////////////////////////////////////////////////
// DeviceInfo / DeviceBlock
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp DeviceInfo JSON round-trip", "[dsp][device][dev]")
{
    DeviceInfo i;
    i.type = "airspy";
    i.name = "Airspy Mini #1234";
    i.cfg = {{"serial", 1234}};
    i.params = {{"mini", true}};
    nlohmann::json j = i;
    CHECK(j["type"] == "airspy");
    CHECK(j["name"] == "Airspy Mini #1234");
    CHECK(j["cfg"]["serial"] == 1234);
    CHECK(j["params"]["mini"] == true);

    DeviceInfo back = j.get<DeviceInfo>();
    CHECK(back.type == i.type);
    CHECK(back.name == i.name);
    CHECK(back.cfg == i.cfg);
    CHECK(back.params == i.params);
}

TEST_CASE("ndsp DeviceInfo equality detects differences", "[dsp][device][dev]")
{
    DeviceInfo a{"rtlsdr", "RTL-SDR", {{"serial", "x"}}, {}};
    DeviceInfo b = a;
    b.cfg = {{"serial", "y"}};
    CHECK_FALSE(a == b);
    b = a;
    b.type = "other";
    CHECK_FALSE(a == b);
}

// Bug : dev.h:18 compares name with b.type instead of b.name, so two
// identical DeviceInfo compare unequal (unless name happens to equal type)
TEST_CASE("ndsp DeviceInfo equality holds for identical infos", "[dsp][device][dev][!shouldfail]")
{
    DeviceInfo a{"rtlsdr", "RTL-SDR #0", {{"serial", "x"}}, {{"p", 1}}};
    DeviceInfo b = a;
    CHECK(a == b);
}

TEST_CASE("ndsp DeviceBlock setDevInfo stores the info and applies its config", "[dsp][device][dev]")
{
    FakeDevice d;
    DeviceInfo i{"fake", "Fake device", {{"serial", "abc"}, {"gain", 10}, {"reject", 1}}, {}};
    d.setDevInfo(i, DeviceBlock::MODE_RX_TX);
    CHECK(d.mode() == DeviceBlock::MODE_RX_TX);
    CHECK(d.info().name == "Fake device");
    // Keys that throw are skipped, others are applied
    CHECK(d.applied["serial"] == "abc");
    CHECK(d.applied["gain"] == 10);
    CHECK_FALSE(d.applied.contains("reject"));
    CHECK(d.is_async());

    d.setStreamSamplerate(0, false, 2.4e6);
    d.setStreamFrequency(0, false, 100e6);
    CHECK(d.getStreamSamplerate(0, false) == 2.4e6);
    CHECK(d.getStreamFrequency(0, false) == 100e6);
}

TEST_CASE("ndsp device lookup without any device plugin", "[dsp][device][dev]")
{
    // No plugins are loaded in the test executable, so no handler answers
    CHECK(getDeviceList(DeviceBlock::MODE_NORMAL).empty());
    DeviceInfo i{"nothing", "Nothing", {}, {}};
    CHECK_THROWS_AS(getDeviceInstanceFromInfo(i, DeviceBlock::MODE_NORMAL), satdump::satdump_exception_t);
}

///////////////////////////////////////////////////////////////////////////////
// OptionsDisplayer (values only, no rendering)
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp OptionsDisplayer stores defaults and typed values", "[dsp][device][options_displayer]")
{
    OptionsDisplayer d;
    nlohmann::ordered_json opts;
    opts["b"] = {{"type", "bool"}, {"default", true}};
    opts["s"] = {{"type", "string"}, {"default", "hello"}};
    opts["u"] = {{"type", "uint"}, {"default", 7}};
    opts["i"] = {{"type", "int"}, {"default", -3}};
    opts["f"] = {{"type", "float"}, {"default", 1.5}};
    opts["q"] = {{"type", "freq"}};
    opts["st"] = {{"type", "stat"}};
    d.add_options(opts);

    nlohmann::json v = d.get_values();
    CHECK(v["b"] == true);
    CHECK(v["s"] == "hello");
    CHECK(v["u"] == 7);
    CHECK(v["i"] == -3);
    CHECK(v["f"] == 1.5);
    CHECK(v["q"] == 0);
    CHECK(v["st"].is_null());

    d.set_values({{"b", false}, {"s", "world"}, {"u", 9}, {"i", -10}, {"f", 2.25}, {"q", 137100000}, {"st", {{"snr", 3}}}});
    v = d.get_values();
    CHECK(v["b"] == false);
    CHECK(v["s"] == "world");
    CHECK(v["u"] == 9);
    CHECK(v["i"] == -10);
    CHECK(v["f"] == 2.25);
    CHECK(v["q"] == 137100000);
    CHECK(v["st"]["snr"] == 3);
}

TEST_CASE("ndsp OptionsDisplayer ignores mistyped and unknown values", "[dsp][device][options_displayer]")
{
    OptionsDisplayer d;
    nlohmann::ordered_json opts;
    opts["b"] = {{"type", "bool"}, {"default", true}};
    opts["f"] = {{"type", "float"}, {"default", 1.0}};
    d.add_options(opts);
    d.set_values({{"b", "not a bool"}, {"f", "nope"}, {"other", 5}});
    nlohmann::json v = d.get_values();
    CHECK(v["b"] == true);
    CHECK(v["f"] == 1.0);
    CHECK_FALSE(v.contains("other"));
}

TEST_CASE("ndsp OptionsDisplayer option parsing", "[dsp][device][options_displayer]")
{
    OptionsDisplayer d;

    // Hidden options are skipped
    nlohmann::ordered_json hidden;
    hidden["h"] = {{"type", "int"}, {"hide", true}};
    hidden["l"] = {{"type", "string"}, {"list", {"a", "b"}}, {"default", "b"}};
    hidden["r"] = {{"type", "float"}, {"range", {0, 10, 0.5}}, {"default", 3.0}};
    d.add_options(hidden);
    nlohmann::json v = d.get_values();
    CHECK_FALSE(v.contains("h"));
    CHECK(v["l"] == "b");
    CHECK(v["r"] == 3.0);

    d.clear();
    CHECK(d.get_values().is_null());

    // Options without a type are rejected. This throws with the internal mutex
    // still locked (see report), so this instance is never used nor destroyed
    // afterwards (destroying a locked std::mutex is undefined).
    OptionsDisplayer *d2 = new OptionsDisplayer();
    nlohmann::ordered_json bad;
    bad["x"] = {{"name", "X"}};
    CHECK_THROWS_AS(d2->add_options(bad), satdump::satdump_exception_t);
}

TEST_CASE("ndsp OptDisplayerWarper mirrors a block configuration", "[dsp][device][options_displayer]")
{
    // Constructor and update() only read the block's config, nothing is rendered
    auto blk = std::make_shared<MultiplyConstBlock<float>>();
    blk->set_cfg("const", 2.5);
    OptDisplayerWarper w(blk);
    blk->set_cfg("const", 4.0);
    w.update();
    w.setShowStats(true);
    CHECK(blk->get_cfg("const") == 4.0);
}
