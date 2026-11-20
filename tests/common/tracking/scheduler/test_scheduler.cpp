#include "common/tracking/scheduler/scheduler.h"
#include <catch2/catch_test_macros.hpp>

// AutoTrackScheduler itself needs the global TLE database and event bus, and TrackedObject owns a
// UI pipeline selector, so only the plain configuration struct is tested here.

TEST_CASE("AutoTrackCfg defaults", "[common][tracking][scheduler]")
{
    satdump::AutoTrackCfg cfg;
    CHECK(cfg.autotrack_min_elevation == 0);
    CHECK_FALSE(cfg.stop_sdr_when_idle);
    CHECK_FALSE(cfg.multi_mode);
    CHECK_FALSE(cfg.use_localtime);
}

TEST_CASE("AutoTrackCfg JSON serialisation round-trips", "[common][tracking][scheduler]")
{
    satdump::AutoTrackCfg cfg;
    cfg.autotrack_min_elevation = 12.5;
    cfg.stop_sdr_when_idle = true;
    cfg.multi_mode = true;
    cfg.use_localtime = false;

    nlohmann::ordered_json j = cfg;
    CHECK(j["autotrack_min_elevation"].get<float>() == 12.5f);
    CHECK(j["stop_sdr_when_idle"] == true);
    CHECK(j["multi_mode"] == true);
    CHECK(j["use_localtime"] == false);

    satdump::AutoTrackCfg back = j.get<satdump::AutoTrackCfg>();
    CHECK(back.autotrack_min_elevation == 12.5f);
    CHECK(back.stop_sdr_when_idle);
    CHECK(back.multi_mode);
    CHECK_FALSE(back.use_localtime);
}

TEST_CASE("AutoTrackCfg JSON deserialisation keeps defaults for missing fields", "[common][tracking][scheduler]")
{
    nlohmann::ordered_json j = nlohmann::ordered_json::parse(R"({"multi_mode": true})");
    satdump::AutoTrackCfg cfg = j.get<satdump::AutoTrackCfg>();
    CHECK(cfg.multi_mode);
    CHECK(cfg.autotrack_min_elevation == 0);
    CHECK_FALSE(cfg.stop_sdr_when_idle);
    CHECK_FALSE(cfg.use_localtime);

    satdump::AutoTrackCfg empty = nlohmann::ordered_json::object().get<satdump::AutoTrackCfg>();
    CHECK_FALSE(empty.multi_mode);
}
