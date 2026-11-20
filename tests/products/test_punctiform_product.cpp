#include "common/tracking/tracking.h"
#include "products/products_test_helpers.h"
#include "products/punctiform_product.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using products_test::TempDir;
using satdump::products::PunctiformProduct;

namespace
{
    // ISS (ZARYA) 2008-09-20 element set, as used by tests/common/tracking
    const satdump::TLE ISS_TLE = {25544, "ISS (ZARYA)", "1 25544U 98067A   08264.51782528 -.00002182  00000-0 -11606-4 0  2927",
                                  "2 25544  51.6416 247.4627 0006703 130.5360 325.0288 15.72125391563537", 0};

    PunctiformProduct::DataHolder channel(std::string name, int n, bool with_positions)
    {
        PunctiformProduct::DataHolder d;
        d.channel_name = name;
        for (int i = 0; i < n; i++)
        {
            d.timestamps.push_back(1221913540.0 + 10 * i);
            d.data.push_back(i * 1.5 - 3);
            if (with_positions)
                d.positions.push_back(geodetic::geodetic_coords_t(10.0 * i - 20, 5.0 * i + 100, 800 + i));
        }
        return d;
    }
} // namespace

TEST_CASE("PunctiformProduct channel lookup by name", "[products][punctiform]")
{
    PunctiformProduct p;
    CHECK(p.type == "punctiform");
    p.data.push_back(channel("temp", 3, true));
    p.data.push_back(channel("pressure", 2, true));
    CHECK(p.getChannelIndexByName("temp") == 0);
    CHECK(p.getChannelIndexByName("pressure") == 1);
    CHECK(&p.getChannelByName("pressure") == &p.data[1]);
    CHECK_THROWS(p.getChannelIndexByName("missing"));
    CHECK_THROWS(p.getChannelByName("missing"));
}

TEST_CASE("PunctiformProduct save/load round-trips every channel field", "[products][punctiform]")
{
    TempDir dir("punctiform");
    nlohmann::json tle = ISS_TLE;
    {
        PunctiformProduct p;
        p.instrument_name = "sounder";
        p.set_tle(tle);
        p.data.push_back(channel("a", 4, true));
        p.data.push_back(channel("b", 3, false));
        p.save(dir.str());
    }

    PunctiformProduct q;
    q.load((dir.path / "product.cbor").string());
    CHECK(q.instrument_name == "sounder");
    REQUIRE(q.has_tle());
    CHECK(q.get_tle() == tle);
    REQUIRE(q.data.size() == 2);

    auto ref_a = channel("a", 4, true);
    auto ref_b = channel("b", 3, false);
    CHECK(q.data[0].channel_name == "a");
    CHECK(q.data[0].timestamps == ref_a.timestamps);
    CHECK(q.data[0].data == ref_a.data);
    REQUIRE(q.data[0].positions.size() == 4);
    for (int i = 0; i < 4; i++)
    {
        CHECK(q.data[0].positions[i].lat == ref_a.positions[i].lat);
        CHECK(q.data[0].positions[i].lon == ref_a.positions[i].lon);
        CHECK(q.data[0].positions[i].alt == ref_a.positions[i].alt);
    }
    CHECK(q.data[1].channel_name == "b");
    CHECK(q.data[1].data == ref_b.data);
    CHECK(q.data[1].positions.empty());
}

TEST_CASE("PunctiformProduct get_sample_position uses stored positions first", "[products][punctiform]")
{
    PunctiformProduct p;
    p.set_tle(ISS_TLE); // Ignored when positions are present
    p.data.push_back(channel("a", 3, true));
    auto pos = p.get_sample_position(0, 2);
    CHECK(pos.lat == 0);
    CHECK(pos.lon == 110);
    CHECK(pos.alt == 802);
}

TEST_CASE("PunctiformProduct get_sample_position propagates the TLE without positions", "[products][punctiform]")
{
    PunctiformProduct p;
    p.set_tle(ISS_TLE);
    p.data.push_back(channel("a", 3, false));

    satdump::SatelliteTracker tracker(ISS_TLE);
    for (int i = 0; i < 3; i++)
    {
        auto pos = p.get_sample_position(0, i);
        auto ref = tracker.get_sat_position_at(p.data[0].timestamps[i]);
        CHECK_THAT(pos.lat, WithinAbs(ref.lat, 1e-9));
        CHECK_THAT(pos.lon, WithinAbs(ref.lon, 1e-9));
        CHECK_THAT(pos.alt, WithinAbs(ref.alt, 1e-6));
    }
    // ISS orbit: inclination 51.6 degrees, altitude around 350 km in 2008
    auto pos = p.get_sample_position(0, 0);
    CHECK(std::fabs(pos.lat) <= 51.7);
    CHECK(pos.alt > 300);
    CHECK(pos.alt < 400);
}

TEST_CASE("PunctiformProduct get_sample_position rejects invalid requests", "[products][punctiform]")
{
    PunctiformProduct p;
    p.data.push_back(channel("a", 3, false));
    CHECK_THROWS(p.get_sample_position(1, 0));  // No such channel
    CHECK_THROWS(p.get_sample_position(-1, 0)); // Negative channel
    CHECK_THROWS(p.get_sample_position(0, 4));  // Past the end
    CHECK_THROWS(p.get_sample_position(0, 1));  // No positions and no TLE
}

// Bug: punctiform_product.cpp:32 checks `sample_index > ch.data.size()` instead of `>=`,
// so the one-past-the-end index is accepted and read out of bounds of positions/timestamps.
// (Here positions has an extra element so the out-of-range read stays defined.)
TEST_CASE("PunctiformProduct get_sample_position rejects the one-past-the-end sample", "[products][punctiform][!shouldfail]")
{
    PunctiformProduct p;
    auto ch = channel("a", 3, true);
    ch.positions.push_back(geodetic::geodetic_coords_t(1, 2, 3));
    p.data.push_back(ch);
    CHECK_THROWS(p.get_sample_position(0, 3));
}
