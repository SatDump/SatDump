#include "db/db_handler.h"
#include "db/iers/iers_handler.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

// IersDBHandler::init() is not called: it registers a handler capturing `this` on the global
// eventBus (which cannot be removed) and schedules a download on the global taskScheduler. The
// tables are created here with the same schema instead, and updateIERS() (network) is not tested.

using Catch::Approx;

namespace
{
    struct IersDb
    {
        std::shared_ptr<satdump::DBHandler> db = std::make_shared<satdump::DBHandler>(":memory:");
        satdump::IersDBHandler iers{db};

        IersDb()
        {
            db->init();
            if (db->run_sql("CREATE TABLE IF NOT EXISTS iers(time INT PRIMARY KEY NOT NULL, a_pole_x FLOAT NOT NULL, a_pole_y FLOAT NOT NULL, ut1_utc FLOAT NOT NULL);") ||
                db->run_sql("CREATE TABLE IF NOT EXISTS leap_seconds(time INT PRIMARY KEY NOT NULL, leap_seconds INT NOT NULL);"))
                throw std::runtime_error("Could not create the test IERS tables");
        }

        void addIers(long t, double x, double y, double ut1)
        {
            if (db->run_sql("INSERT INTO iers VALUES (" + std::to_string(t) + ", " + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(ut1) + ");"))
                throw std::runtime_error("insert failed");
        }

        void addLeap(long t, int leaps)
        {
            if (db->run_sql("INSERT INTO leap_seconds VALUES (" + std::to_string(t) + ", " + std::to_string(leaps) + ");"))
                throw std::runtime_error("insert failed");
        }
    };

    // Unix times of real leap second introductions, with TAI-UTC after each (IERS Bulletin C)
    const long LS_2009 = 1230768000; // 2009-01-01, 34 s
    const long LS_2012 = 1341100800; // 2012-07-01, 35 s
    const long LS_2015 = 1435708800; // 2015-07-01, 36 s
    const long LS_2017 = 1483228800; // 2017-01-01, 37 s
} // namespace

TEST_CASE("IersDBHandler getBestIERSInfo converts polar motion to milliarcseconds", "[db][iers]")
{
    IersDb t;
    t.addIers(1700000000, 0.123456, -0.25, -0.0123);
    t.addLeap(LS_2017, 37);

    satdump::IERSInfo info = t.iers.getBestIERSInfo(1700000000);
    CHECK(info.polar_dx == Approx(123.456));
    CHECK(info.polar_dy == Approx(-250.0));
    CHECK(info.ut1_utc == Approx(-0.0123));
    CHECK(info.leap_seconds == 37);
}

TEST_CASE("IersDBHandler getBestIERSInfo picks the leap seconds in force at the time", "[db][iers]")
{
    IersDb t;
    t.addIers(1400000000, 0.1, 0.2, 0.3);
    // Inserted out of order on purpose
    t.addLeap(LS_2015, 36);
    t.addLeap(LS_2009, 34);
    t.addLeap(LS_2017, 37);
    t.addLeap(LS_2012, 35);

    CHECK(t.iers.getBestIERSInfo(LS_2009).leap_seconds == 34);
    CHECK(t.iers.getBestIERSInfo(LS_2012 - 1).leap_seconds == 34);
    CHECK(t.iers.getBestIERSInfo(LS_2012).leap_seconds == 35);
    CHECK(t.iers.getBestIERSInfo(1400000000).leap_seconds == 35);
    CHECK(t.iers.getBestIERSInfo(LS_2015 + 0.5).leap_seconds == 36);
    CHECK(t.iers.getBestIERSInfo(LS_2017 - 0.5).leap_seconds == 36);
    CHECK(t.iers.getBestIERSInfo(1900000000).leap_seconds == 37);
}

// Bug: getBestIERSInfo (iers_handler.cpp:182) orders by `abs(<time>)`, a constant, instead of by the
// distance `abs(time - <time>)`, so it returns whichever row SQLite scans first rather than the
// record closest to the requested time.
TEST_CASE("IersDBHandler getBestIERSInfo returns the record closest in time", "[db][iers][!shouldfail]")
{
    IersDb t;
    t.addLeap(LS_2017, 37);
    t.addIers(1700000000, 0.001, 0.001, 0.001);
    t.addIers(1700086400, 0.002, 0.002, 0.002);
    t.addIers(1700172800, 0.003, 0.003, 0.003);

    CHECK(t.iers.getBestIERSInfo(1700172000).ut1_utc == Approx(0.003));
    CHECK(t.iers.getBestIERSInfo(1700090000).ut1_utc == Approx(0.002));
}
