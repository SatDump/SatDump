#include "db/db_handler.h"
#include "db/kepler/kepler_handler.h"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <stdexcept>

// KeplerDBHandler::init() is not called: it needs the global SatDump config and registers an
// auto-update on the global eventBus/taskScheduler. The table is created here with the same schema,
// as in tests/src-core/common/tracking/scheduler/test_passes.cpp. Nothing touches satdump::db_keplers.

using Catch::Matchers::WithinRel;

namespace
{
    struct KeplerDb
    {
        std::shared_ptr<satdump::DBHandler> db = std::make_shared<satdump::DBHandler>(":memory:");
        satdump::KeplerDBHandler kep{db};

        KeplerDb()
        {
            db->init();
            if (db->run_sql("CREATE TABLE IF NOT EXISTS kepler("
                            "id TEXT PRIMARY KEY NOT NULL, satellite_number INT NOT NULL, element_number INT NOT NULL,"
                            "name TEXT NOT NULL, designator TEXT NOT NULL, epoch REAL NOT NULL, inclination REAL NOT NULL,"
                            "right_ascension REAL NOT NULL, eccentricity REAL NOT NULL, argument_of_perigee REAL NOT NULL,"
                            "mean_anomaly REAL NOT NULL, mean_motion REAL NOT NULL, derivative_mean_motion REAL NOT NULL,"
                            "second_derivative_mean_motion REAL NOT NULL, bstar_drag_term REAL NOT NULL, revolutions_at_epoch INT NOT NULL);"))
                throw std::runtime_error("Could not create the test Kepler table");
        }
    };

    // ISS (ZARYA) 2008-09-20 element set (1 25544U 98067A   08264.51782528 ...)
    satdump::KeplerData iss()
    {
        satdump::KeplerData k;
        k.satellite_number = 25544;
        k.element_number = 292;
        k.name = "ISS (ZARYA)";
        k.designator = "1998-067A";
        k.epoch = 1221913540.104192;
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

    satdump::KeplerData variant(int norad, double epoch, long element_number)
    {
        satdump::KeplerData k = iss();
        k.satellite_number = norad;
        k.epoch = epoch;
        k.element_number = element_number;
        k.mean_anomaly = 10.0 + element_number;
        return k;
    }

    void checkSame(const satdump::KeplerData &a, const satdump::KeplerData &b)
    {
        CHECK(a.satellite_number == b.satellite_number);
        CHECK(a.element_number == b.element_number);
        CHECK(a.name == b.name);
        CHECK(a.designator == b.designator);
        CHECK_THAT(a.epoch, WithinRel(b.epoch, 1e-14));
        CHECK_THAT(a.inclination, WithinRel(b.inclination, 1e-14));
        CHECK_THAT(a.right_ascension, WithinRel(b.right_ascension, 1e-14));
        CHECK_THAT(a.eccentricity, WithinRel(b.eccentricity, 1e-14));
        CHECK_THAT(a.argument_of_perigee, WithinRel(b.argument_of_perigee, 1e-14));
        CHECK_THAT(a.mean_anomaly, WithinRel(b.mean_anomaly, 1e-14));
        CHECK_THAT(a.mean_motion, WithinRel(b.mean_motion, 1e-14));
        CHECK_THAT(a.derivative_mean_motion, WithinRel(b.derivative_mean_motion, 1e-14));
        CHECK(a.second_derivative_mean_motion == b.second_derivative_mean_motion);
        CHECK_THAT(a.bstar_drag_term, WithinRel(b.bstar_drag_term, 1e-14));
        CHECK(a.revolutions_at_epoch == b.revolutions_at_epoch);
    }
} // namespace

TEST_CASE("KeplerDBHandler putKepler and getKepler round-trip every field", "[db][kepler]")
{
    KeplerDb t;
    t.kep.putKepler(iss());
    CHECK(t.db->get_table_size("kepler") == 1);

    satdump::KeplerData out;
    REQUIRE(t.kep.getKepler(out, 25544, 1221913540));
    checkSame(out, iss());
}

TEST_CASE("KeplerDBHandler getKepler for an unknown satellite returns false", "[db][kepler]")
{
    KeplerDb t;
    t.kep.putKepler(iss());
    satdump::KeplerData out;
    out.satellite_number = -7;
    CHECK_FALSE(t.kep.getKepler(out, 99999, 1221913540));
    CHECK(out.satellite_number == -7);
    CHECK_FALSE(t.kep.get_from_norad(12345).has_value());
    CHECK_FALSE(t.kep.get_from_norad_time(12345, 1221913540).has_value());
}

TEST_CASE("KeplerDBHandler getKepler picks the epoch closest to the requested time", "[db][kepler]")
{
    KeplerDb t;
    const double day = 86400;
    const double e0 = 1600000000;
    t.kep.putKepler(variant(1000, e0, 1));
    t.kep.putKepler(variant(1000, e0 + 10 * day, 2));
    t.kep.putKepler(variant(1000, e0 + 20 * day, 3));
    t.kep.putKepler(variant(2000, e0 + 10 * day, 99)); // Another satellite

    satdump::KeplerData out;
    REQUIRE(t.kep.getKepler(out, 1000, e0 - 100 * day));
    CHECK(out.element_number == 1);
    REQUIRE(t.kep.getKepler(out, 1000, e0 + 4 * day));
    CHECK(out.element_number == 1);
    REQUIRE(t.kep.getKepler(out, 1000, e0 + 6 * day));
    CHECK(out.element_number == 2);
    REQUIRE(t.kep.getKepler(out, 1000, e0 + 16 * day));
    CHECK(out.element_number == 3);
    REQUIRE(t.kep.getKepler(out, 2000, e0));
    CHECK(out.element_number == 99);

    // Without a time the current time is used, so the newest (past) epoch wins
    REQUIRE(t.kep.getKepler(out, 1000));
    CHECK(out.element_number == 3);
}

TEST_CASE("KeplerDBHandler putKepler updates an existing satellite and epoch", "[db][kepler]")
{
    KeplerDb t;
    t.kep.putKepler(iss());
    auto updated = iss();
    updated.element_number = 999;
    updated.name = "ISS (RENAMED)";
    updated.mean_motion = 15.5;
    t.kep.putKepler(updated);

    // Same NORAD id and epoch -> same row
    CHECK(t.db->get_table_size("kepler") == 1);
    satdump::KeplerData out;
    REQUIRE(t.kep.getKepler(out, 25544, iss().epoch));
    CHECK(out.element_number == 999);
    CHECK(out.name == "ISS (RENAMED)");
    CHECK(out.mean_motion == 15.5);
}

TEST_CASE("KeplerDBHandler stores names containing quotes", "[db][kepler]")
{
    KeplerDb t;
    auto k = iss();
    k.name = "SAT 'QUOTED' NAME";
    t.kep.putKepler(k);
    satdump::KeplerData out;
    REQUIRE(t.kep.getKepler(out, 25544, k.epoch));
    CHECK(out.name == "SAT 'QUOTED' NAME");
}

TEST_CASE("KeplerDBHandler getAllNewestKepler returns the newest epoch per satellite", "[db][kepler]")
{
    KeplerDb t;
    CHECK(t.kep.getAllNewestKepler().empty());

    t.kep.putKepler(variant(1000, 1600000000, 1));
    t.kep.putKepler(variant(1000, 1600900000, 2));
    t.kep.putKepler(variant(1000, 1600500000, 3));
    t.kep.putKepler(variant(2000, 1500000000, 10));
    t.kep.putKepler(variant(3000, 1700000000, 20));
    t.kep.putKepler(variant(3000, 1690000000, 21));

    auto all = t.kep.getAllNewestKepler();
    REQUIRE(all.size() == 3);
    std::sort(all.begin(), all.end(), [](auto &a, auto &b) { return a.satellite_number < b.satellite_number; });
    CHECK(all[0].satellite_number == 1000);
    CHECK(all[0].element_number == 2);
    CHECK(all[1].satellite_number == 2000);
    CHECK(all[1].element_number == 10);
    CHECK(all[2].satellite_number == 3000);
    CHECK(all[2].element_number == 20);
    CHECK(all[2].mean_anomaly == 30.0);
}

TEST_CASE("KeplerDBHandler get_from_norad_time converts the stored elements to a TLE", "[db][kepler]")
{
    KeplerDb t;
    t.kep.putKepler(iss());
    auto tle = t.kep.get_from_norad_time(25544, 1221913540);
    REQUIRE(tle.has_value());
    CHECK(tle->norad == 25544);
    CHECK(tle->name == "ISS (ZARYA)");
    CHECK(tle->time == 1221913540);
    CHECK(tle->line1.rfind("1 25544U 98067A  ", 0) == 0);
    CHECK(tle->line2.rfind("2 25544 ", 0) == 0);

    auto tle2 = t.kep.get_from_norad(25544);
    REQUIRE(tle2.has_value());
    CHECK(tle2->line1 == tle->line1);
    CHECK(tle2->line2 == tle->line2);
}
