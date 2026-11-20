#include "common/tracking/obj_tracker/object_tracker.h"
#include <catch2/catch_test_macros.hpp>

// ObjectTracker's constructor needs the global TLE database and event bus and starts worker
// threads, so only its public az/el value type is tested here.

TEST_CASE("ObjectTracker::SatAzEl defaults to zero", "[common][tracking][obj_tracker]")
{
    satdump::ObjectTracker::SatAzEl p;
    CHECK(p.az == 0);
    CHECK(p.el == 0);
}

TEST_CASE("ObjectTracker::SatAzEl JSON serialisation round-trips", "[common][tracking][obj_tracker]")
{
    satdump::ObjectTracker::SatAzEl p;
    p.az = 271.25f;
    p.el = -3.5f;
    nlohmann::json j = p;
    CHECK(j["az"].get<float>() == 271.25f);
    CHECK(j["el"].get<float>() == -3.5f);

    satdump::ObjectTracker::SatAzEl back = j.get<satdump::ObjectTracker::SatAzEl>();
    CHECK(back.az == 271.25f);
    CHECK(back.el == -3.5f);

    CHECK_THROWS(nlohmann::json::parse(R"({"az": 1.0})").get<satdump::ObjectTracker::SatAzEl>());
}
