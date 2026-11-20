#include "common/tracking/scheduler/passes.h"
#include "common/tracking/tracking.h"
#include "db/db_handler.h"
#include "db/kepler/kepler_handler.h"
#include "init.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <stdexcept>

using Catch::Matchers::WithinAbs;

namespace
{
    // getPassesForSatellite looks TLEs up in the global satdump::db_keplers. This fixture points it
    // at an in-memory SQLite database holding only the TLEs a test adds, and restores the previous
    // (normally null) handler afterwards.
    class KeplerDbFixture
    {
    private:
        std::shared_ptr<satdump::KeplerDBHandler> previous;
        std::shared_ptr<satdump::DBHandler> db;

    public:
        KeplerDbFixture()
        {
            previous = satdump::db_keplers;
            db = std::make_shared<satdump::DBHandler>(":memory:");
            db->init();
            // Same schema as KeplerDBHandler::init(), which cannot be called here because it
            // needs the SatDump config and task scheduler
            if (db->run_sql("CREATE TABLE IF NOT EXISTS kepler("
                            "id TEXT PRIMARY KEY NOT NULL, satellite_number INT NOT NULL, element_number INT NOT NULL,"
                            "name TEXT NOT NULL, designator TEXT NOT NULL, epoch REAL NOT NULL, inclination REAL NOT NULL,"
                            "right_ascension REAL NOT NULL, eccentricity REAL NOT NULL, argument_of_perigee REAL NOT NULL,"
                            "mean_anomaly REAL NOT NULL, mean_motion REAL NOT NULL, derivative_mean_motion REAL NOT NULL,"
                            "second_derivative_mean_motion REAL NOT NULL, bstar_drag_term REAL NOT NULL, revolutions_at_epoch INT NOT NULL);"))
                throw std::runtime_error("Could not create the test Kepler table");
            satdump::db_keplers = std::make_shared<satdump::KeplerDBHandler>(db);
        }

        ~KeplerDbFixture() { satdump::db_keplers = previous; }

        void add(satdump::KeplerData kep) { satdump::db_keplers->putKepler(kep); }

        // The TLE getPassesForSatellite will actually use (regenerated from the stored elements)
        satdump::TLE stored(int norad)
        {
            auto tle = satdump::db_keplers->get_from_norad(norad);
            if (!tle.has_value())
                throw std::runtime_error("Test TLE missing from the database");
            return *tle;
        }
    };

    // ISS (ZARYA) 2008-09-20 element set (see test_tle.cpp), as stored Kepler elements
    satdump::KeplerData issKepler()
    {
        satdump::KeplerData k;
        k.satellite_number = 25544;
        k.element_number = 292;
        k.name = "ISS (ZARYA)";
        k.designator = "1998-067A";
        k.epoch = 1221913540.104; // 08264.51782528
        k.inclination = 51.6416;
        k.right_ascension = 247.4627;
        k.eccentricity = 0.0006703;
        k.argument_of_perigee = 130.5360;
        k.mean_anomaly = 325.0288;
        k.mean_motion = 15.72125391;
        k.derivative_mean_motion = -0.00002182;
        k.second_derivative_mean_motion = 0;
        k.bstar_drag_term = -0.11606e-4;
        k.revolutions_at_epoch = 56353;
        return k;
    }

    // Synthetic geostationary satellite (1.0027 rev/day, near-zero inclination and eccentricity)
    satdump::KeplerData geoKepler()
    {
        satdump::KeplerData k = issKepler();
        k.satellite_number = 90001;
        k.element_number = 1;
        k.name = "TEST GEO";
        k.designator = "2020-001A";
        k.epoch = 1577880000; // 2020-01-01 12:00 UTC
        k.inclination = 0.05;
        k.right_ascension = 10;
        k.eccentricity = 0.0001;
        k.argument_of_perigee = 0;
        k.mean_anomaly = 0;
        k.mean_motion = 1.00273791;
        k.derivative_mean_motion = 0;
        k.bstar_drag_term = 0;
        k.revolutions_at_epoch = 100;
        return k;
    }

    const double ISS_START = 1221913540.0;
    const double DAY = 86400;

    // Paris
    const double QTH_LAT = 48.85, QTH_LON = 2.35;

    double elevationAt(satdump::SatelliteTracker &tracker, double t) { return tracker.get_observed_position(t, QTH_LAT, QTH_LON, 0).elevation * RAD_TO_DEG; }

    satdump::SatellitePass pass(int norad, double aos, double los, float el) { return {norad, aos, los, el}; }
} // namespace

TEST_CASE_METHOD(KeplerDbFixture, "getPassesForSatellite returns nothing for an unknown satellite", "[common][tracking][passes]")
{
    auto passes = satdump::getPassesForSatellite(12345, ISS_START, DAY, QTH_LON, QTH_LAT, 0);
    CHECK(passes.empty());
}

TEST_CASE_METHOD(KeplerDbFixture, "getPassesForSatellite ISS passes over one day are well formed", "[common][tracking][passes]")
{
    add(issKepler());
    auto passes = satdump::getPassesForSatellite(25544, ISS_START, DAY, QTH_LON, QTH_LAT, 0);

    // A 51.6 deg LEO seen from 48.85 N rises a handful of times a day (bunched in two groups)
    CHECK(passes.size() >= 3);
    CHECK(passes.size() <= 8);

    for (size_t i = 0; i < passes.size(); i++)
    {
        INFO("pass " << i);
        auto &p = passes[i];
        CHECK(p.norad == 25544);
        CHECK(p.aos_time < p.los_time);
        // At ~350 km a horizon-to-horizon pass lasts at most ~11 minutes
        CHECK(p.los_time - p.aos_time <= 12 * 60);
        CHECK(p.max_elevation > 0);
        CHECK(p.max_elevation <= 90);
        CHECK(p.los_time >= ISS_START);
        if (i + 1 < passes.size())
        {
            // Ordered, non-overlapping, roughly one orbit (~92 min) or more apart
            CHECK(p.los_time < passes[i + 1].aos_time);
            CHECK(passes[i + 1].aos_time - p.aos_time > 60 * 60);
            // Only the last pass may start after the requested window
            CHECK(p.aos_time < ISS_START + DAY);
        }
    }
}

TEST_CASE_METHOD(KeplerDbFixture, "getPassesForSatellite ISS AOS/LOS match independently computed elevations", "[common][tracking][passes]")
{
    add(issKepler());
    auto passes = satdump::getPassesForSatellite(25544, ISS_START, DAY, QTH_LON, QTH_LAT, 0);
    REQUIRE_FALSE(passes.empty());

    satdump::SatelliteTracker tracker(stored(25544));
    for (auto &p : passes)
    {
        INFO("pass AOS " << p.aos_time << " LOS " << p.los_time);
        // Below the horizon just before AOS and just after LOS, above it in between
        CHECK(elevationAt(tracker, p.aos_time - 30) < 0);
        CHECK(elevationAt(tracker, p.aos_time + 30) > 0);
        CHECK(elevationAt(tracker, p.los_time - 30) > 0);
        CHECK(elevationAt(tracker, p.los_time + 30) < 0);

        // Find the culmination (TCA) with 1 s resolution
        double tca = p.aos_time, best = -90;
        for (double t = p.aos_time; t <= p.los_time; t += 1)
        {
            double el = elevationAt(tracker, t);
            if (el > best)
            {
                best = el;
                tca = t;
            }
        }
        CHECK(p.aos_time < tca);
        CHECK(tca < p.los_time);
        // max_elevation is sampled at 51 points, so it can only under-estimate, and by
        // little: near TCA elevation changes slowly over one step (< 15 s)
        CHECK(p.max_elevation <= best + 0.01);
        CHECK(p.max_elevation >= best - 1.0);
    }
}

TEST_CASE_METHOD(KeplerDbFixture, "getPassesForSatellite fills in elevation for premade passes", "[common][tracking][passes]")
{
    add(issKepler());
    auto passes = satdump::getPassesForSatellite(25544, ISS_START, DAY, QTH_LON, QTH_LAT, 0);
    REQUIRE(passes.size() >= 2);

    std::vector<satdump::SatellitePass> premade;
    for (auto &p : passes)
        premade.push_back(pass(-1, p.aos_time, p.los_time, -5));

    auto filled = satdump::getPassesForSatellite(25544, ISS_START, DAY, QTH_LON, QTH_LAT, 0, premade);
    REQUIRE(filled.size() == premade.size());
    for (size_t i = 0; i < filled.size(); i++)
    {
        CHECK(filled[i].norad == 25544);
        CHECK(filled[i].aos_time == premade[i].aos_time);
        CHECK(filled[i].los_time == premade[i].los_time);
        // Same sampling as the normal path, so the same result
        CHECK_THAT(filled[i].max_elevation, WithinAbs(passes[i].max_elevation, 1e-4));
    }
}

TEST_CASE_METHOD(KeplerDbFixture, "getPassesForSatellite treats a visible GEO as one pass spanning the window", "[common][tracking][passes]")
{
    add(geoKepler());
    const double start = 1577880000 + 3600;
    satdump::SatelliteTracker tracker(stored(90001));
    geodetic::geodetic_coords_t sub = tracker.get_sat_position_at(start);
    REQUIRE(std::fabs(sub.lat) < 1.0);

    auto passes = satdump::getPassesForSatellite(90001, start, DAY, sub.lon, sub.lat, 0);
    REQUIRE(passes.size() == 1);
    CHECK(passes[0].norad == 90001);
    CHECK(passes[0].aos_time == start);
    CHECK(passes[0].los_time == start + DAY);
    CHECK(passes[0].max_elevation > 85);

    // From the other side of the Earth it is never visible
    auto none = satdump::getPassesForSatellite(90001, start, DAY, sub.lon + 180, 0, 0);
    CHECK(none.empty());
}

TEST_CASE("filterPassesByElevation keeps passes within inclusive bounds", "[common][tracking][passes]")
{
    std::vector<satdump::SatellitePass> passes = {pass(1, 0, 10, 5), pass(2, 20, 30, 10), pass(3, 40, 50, 45.5), pass(4, 60, 70, 80), pass(5, 80, 90, 90)};
    auto f = satdump::filterPassesByElevation(passes, 10, 80);
    REQUIRE(f.size() == 3);
    CHECK(f[0].norad == 2);
    CHECK(f[1].norad == 3);
    CHECK(f[2].norad == 4);

    CHECK(satdump::filterPassesByElevation(passes, 0, 90).size() == 5);
    CHECK(satdump::filterPassesByElevation(passes, 91, 100).empty());
    CHECK(satdump::filterPassesByElevation({}, 0, 90).empty());
}

TEST_CASE("selectPassesForAutotrack keeps all non-overlapping passes in order", "[common][tracking][passes]")
{
    std::vector<satdump::SatellitePass> passes = {pass(1, 1000, 1600, 20), pass(2, 2000, 2500, 70), pass(3, 3000, 3300, 5)};
    auto sel = satdump::selectPassesForAutotrack(passes);
    REQUIRE(sel.size() == 3);
    CHECK(sel[0].norad == 1);
    CHECK(sel[1].norad == 2);
    CHECK(sel[2].norad == 3);
}

TEST_CASE("selectPassesForAutotrack resolves overlaps in favour of the higher pass", "[common][tracking][passes]")
{
    SECTION("later pass is higher")
    {
        auto sel = satdump::selectPassesForAutotrack({pass(1, 1000, 1600, 30), pass(2, 1300, 1900, 60), pass(3, 2500, 2800, 10)});
        REQUIRE(sel.size() == 2);
        CHECK(sel[0].norad == 2);
        CHECK(sel[1].norad == 3);
    }
    SECTION("earlier pass is higher")
    {
        auto sel = satdump::selectPassesForAutotrack({pass(1, 1000, 1600, 60), pass(2, 1300, 1900, 30)});
        REQUIRE(sel.size() == 1);
        CHECK(sel[0].norad == 1);
    }
    SECTION("chain of overlaps")
    {
        // 2 beats 1 and 3; 5 beats 4
        std::vector<satdump::SatellitePass> passes = {pass(1, 1000, 1500, 10), pass(2, 1400, 2000, 50), pass(3, 1900, 2300, 20), pass(4, 2600, 3000, 40), pass(5, 2900, 3100, 45)};
        auto sel = satdump::selectPassesForAutotrack(passes);
        REQUIRE(sel.size() == 2);
        CHECK(sel[0].norad == 2);
        CHECK(sel[1].norad == 5);
    }
}

TEST_CASE("selectPassesForAutotrack of nothing is nothing", "[common][tracking][passes]") { CHECK(satdump::selectPassesForAutotrack({}).empty()); }
