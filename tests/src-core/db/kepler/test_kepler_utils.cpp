#include "db/kepler/kepler_handler.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <string>

// Only the offline conversion helpers are tested; tryFetchOMMFileFromURL and
// tryFetchSingleOMMwithNorad download from the internet.

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

namespace
{
    // ISS (ZARYA) 2008 element set, as a TLE and as the equivalent CelesTrak OMM CSV record.
    // Epoch 08264.51782528 = 2008-09-20T12:25:40.104192Z = 1221913540.104192 Unix time.
    const std::string ISS_L1 = "1 25544U 98067A   08264.51782528 -.00002182  00000-0 -11606-4 0  2927";
    const std::string ISS_L2 = "2 25544  51.6416 247.4627 0006703 130.5360 325.0288 15.72125391563537";
    const double ISS_EPOCH = 1221913540.104192;

    const std::string OMM_HEADER = "OBJECT_NAME,OBJECT_ID,EPOCH,MEAN_MOTION,ECCENTRICITY,INCLINATION,RA_OF_ASC_NODE,ARG_OF_PERICENTER,MEAN_ANOMALY,"
                                   "EPHEMERIS_TYPE,CLASSIFICATION_TYPE,NORAD_CAT_ID,ELEMENT_SET_NO,REV_AT_EPOCH,BSTAR,MEAN_MOTION_DOT,MEAN_MOTION_DDOT";
    const std::string ISS_OMM = "ISS (ZARYA),1998-067A,2008-09-20T12:25:40.104192,15.72125391,.0006703,51.6416,247.4627,130.5360,325.0288,0,U,25544,292,56353,-.11606E-4,-.00002182,0";
    const std::string NOAA19_OMM = "NOAA 19,2009-005A,2024-01-02T03:04:05.500000,14.12801793,.0013766,99.0486,104.2436,214.9339,145.0961,0,U,33591,999,77312,.11745E-3,.00000256,0";

    satdump::KeplerData issKepler()
    {
        satdump::KeplerData k;
        k.satellite_number = 25544;
        k.element_number = 292;
        k.name = "ISS (ZARYA)";
        k.designator = "1998-067A";
        k.epoch = ISS_EPOCH;
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

    void checkIss(const satdump::KeplerData &k)
    {
        CHECK(k.satellite_number == 25544);
        CHECK(k.element_number == 292);
        CHECK(k.revolutions_at_epoch == 56353);
        CHECK_THAT(k.epoch, WithinAbs(ISS_EPOCH, 1e-3));
        CHECK_THAT(k.inclination, WithinAbs(51.6416, 1e-9));
        CHECK_THAT(k.right_ascension, WithinAbs(247.4627, 1e-9));
        CHECK_THAT(k.eccentricity, WithinAbs(0.0006703, 1e-12));
        CHECK_THAT(k.argument_of_perigee, WithinAbs(130.5360, 1e-9));
        CHECK_THAT(k.mean_anomaly, WithinAbs(325.0288, 1e-9));
        CHECK_THAT(k.mean_motion, WithinAbs(15.72125391, 1e-9));
        CHECK_THAT(k.derivative_mean_motion, WithinRel(-0.00002182, 1e-9));
        CHECK(k.second_derivative_mean_motion == 0.0);
        CHECK_THAT(k.bstar_drag_term, WithinRel(-0.11606e-4, 1e-9));
    }

    satdump::KeplerData roundTrip(const satdump::KeplerData &k)
    {
        satdump::KeplerData out;
        if (!satdump::tleToKepler(satdump::keplerToTle(k), out))
            throw std::runtime_error("tleToKepler failed");
        return out;
    }
} // namespace

TEST_CASE("ccsdsOmmToKepler parses a CelesTrak CSV record", "[db][kepler_utils]")
{
    satdump::KeplerData k;
    REQUIRE(satdump::ccsdsOmmToKepler(ISS_OMM, k));
    CHECK(k.name == "ISS (ZARYA)");
    CHECK(k.designator == "1998-067A");
    checkIss(k);
    CHECK_THAT(k.epoch, WithinAbs(ISS_EPOCH, 1e-5));
}

TEST_CASE("ccsdsOmmToKepler rejects headers and malformed records", "[db][kepler_utils]")
{
    satdump::KeplerData k;
    CHECK_FALSE(satdump::ccsdsOmmToKepler(OMM_HEADER, k));
    CHECK_FALSE(satdump::ccsdsOmmToKepler("", k));
    CHECK_FALSE(satdump::ccsdsOmmToKepler("ISS,1998-067A,2008-09-20T12:25:40", k));
    // One field too many
    CHECK_FALSE(satdump::ccsdsOmmToKepler(ISS_OMM + ",extra", k));
    // Non-numeric orbital element
    std::string bad = ISS_OMM;
    bad.replace(bad.find("15.72125391"), 11, "fast");
    CHECK_FALSE(satdump::ccsdsOmmToKepler(bad, k));
}

// Bug: ccsdsOmmToKepler (kepler_utils.cpp:149-154) only sets the epoch when the EPOCH field parses,
// but still returns true otherwise, leaving kep.epoch with whatever it held before (uninitialised in
// real callers).
TEST_CASE("ccsdsOmmToKepler rejects a record with an invalid epoch", "[db][kepler_utils][!shouldfail]")
{
    std::string bad = ISS_OMM;
    bad.replace(bad.find("2008-09-20T12:25:40.104192"), 26, "not a date");
    satdump::KeplerData k;
    k.epoch = -12345;
    CHECK_FALSE(satdump::ccsdsOmmToKepler(bad, k));
}

TEST_CASE("parseCcsdsOmmFile keeps only valid records", "[db][kepler_utils]")
{
    std::string file = OMM_HEADER + "\n" + ISS_OMM + "\n\ngarbage line\n" + NOAA19_OMM + "\n";
    auto keps = satdump::parseCcsdsOmmFile(file);
    REQUIRE(keps.size() == 2);
    checkIss(keps[0]);
    CHECK(keps[1].name == "NOAA 19");
    CHECK(keps[1].satellite_number == 33591);
    CHECK(keps[1].element_number == 999);
    CHECK_THAT(keps[1].epoch, WithinAbs(1704164645.5, 1e-5)); // 2024-01-02T03:04:05.5Z
    CHECK_THAT(keps[1].bstar_drag_term, WithinRel(0.11745e-3, 1e-9));

    CHECK(satdump::parseCcsdsOmmFile("").empty());
    CHECK(satdump::parseCcsdsOmmFile(OMM_HEADER).empty());
}

TEST_CASE("parseCcsdsOmmFile accepts CRLF line endings", "[db][kepler_utils]")
{
    auto keps = satdump::parseCcsdsOmmFile(OMM_HEADER + "\r\n" + ISS_OMM + "\r\n" + NOAA19_OMM + "\r\n");
    REQUIRE(keps.size() == 2);
    checkIss(keps[0]);
    CHECK(keps[1].satellite_number == 33591);
}

TEST_CASE("tleToKepler parses the ISS reference TLE", "[db][kepler_utils]")
{
    satdump::TLE tle;
    tle.norad = 25544;
    tle.name = "ISS (ZARYA)";
    tle.line1 = ISS_L1;
    tle.line2 = ISS_L2;

    satdump::KeplerData k;
    REQUIRE(satdump::tleToKepler(tle, k));
    CHECK(k.name == "ISS (ZARYA)");
    // The designator is taken from columns 10-17 with spaces removed
    CHECK(k.designator == "98067A");
    checkIss(k);
}

// Bug: tleToKepler (kepler_utils.cpp:196) always maps the two digit epoch year to 20YY. The TLE
// convention is 57-99 -> 19YY, 00-56 -> 20YY, so pre-2000 element sets land a century late.
TEST_CASE("tleToKepler maps epoch years 57-99 to the 1900s", "[db][kepler_utils][!shouldfail]")
{
    satdump::TLE tle;
    tle.name = "ISS 1998";
    tle.line1 = "1 25544U 98067A   98264.51782528 -.00002182  00000-0 -11606-4 0  2927";
    tle.line2 = ISS_L2;
    satdump::KeplerData k;
    REQUIRE(satdump::tleToKepler(tle, k));
    // 1998-09-21T12:25:40.104192Z
    CHECK_THAT(k.epoch, WithinAbs(906380740.104192, 1e-3));
}

TEST_CASE("keplerToTle produces two 69 character lines", "[db][kepler_utils]")
{
    auto tle = satdump::keplerToTle(issKepler());
    CHECK(tle.norad == 25544);
    CHECK(tle.name == "ISS (ZARYA)");
    CHECK(tle.time == 1221913540);
    CHECK(tle.line1.size() == 69);
    CHECK(tle.line2.size() == 69);
    CHECK(tle.line1.rfind("1 25544U 98067A  ", 0) == 0);
    CHECK(tle.line2.rfind("2 25544 ", 0) == 0);
    // Epoch day of year, first derivative, BSTAR and element number columns
    CHECK(tle.line1.substr(20, 12) == "264.51782528");
    CHECK(tle.line1.substr(33, 10) == "-.00002182");
    CHECK(tle.line1.substr(53, 8) == "-11606-4");
    CHECK(tle.line1.substr(64, 4) == " 292");
    CHECK(tle.line2.substr(63, 5) == "56353");
    // No checksum is computed: both lines end with a literal 'C' placeholder
    CHECK(tle.line1.back() == 'C');
    CHECK(tle.line2.back() == 'C');
}

TEST_CASE("keplerToTle clamps catalog numbers above 99999 in the lines", "[db][kepler_utils]")
{
    auto k = issKepler();
    k.satellite_number = 270000;
    auto tle = satdump::keplerToTle(k);
    CHECK(tle.norad == 270000);
    CHECK(tle.line1.substr(2, 5) == "99999");
    CHECK(tle.line2.substr(2, 5) == "99999");
}

TEST_CASE("keplerToTle and tleToKepler round-trip approximately", "[db][kepler_utils]")
{
    auto k = issKepler();
    auto r = roundTrip(k);
    CHECK(r.satellite_number == k.satellite_number);
    CHECK(r.element_number == k.element_number);
    CHECK(r.revolutions_at_epoch == k.revolutions_at_epoch);
    CHECK(r.designator == "98067A");
    // The epoch is carried as day of year with 8 decimals (< 1 ms)
    CHECK_THAT(r.epoch, WithinAbs(k.epoch, 1e-3));
    CHECK_THAT(r.inclination, WithinAbs(k.inclination, 2e-4));
    CHECK_THAT(r.right_ascension, WithinAbs(k.right_ascension, 2e-4));
    CHECK_THAT(r.eccentricity, WithinAbs(k.eccentricity, 2e-7));
    CHECK_THAT(r.argument_of_perigee, WithinAbs(k.argument_of_perigee, 2e-4));
    CHECK_THAT(r.mean_anomaly, WithinAbs(k.mean_anomaly, 2e-4));
    CHECK_THAT(r.mean_motion, WithinAbs(k.mean_motion, 2e-8));
    CHECK_THAT(r.derivative_mean_motion, WithinRel(k.derivative_mean_motion, 1e-6));
    CHECK_THAT(r.bstar_drag_term, WithinRel(k.bstar_drag_term, 1e-6));

    SECTION("a recent epoch with a full four digit year")
    {
        k.epoch = 1704164645.5; // 2024-01-02T03:04:05.5Z = day 2.12784143...
        auto tle = satdump::keplerToTle(k);
        CHECK(tle.line1.substr(18, 2) == "24");
        CHECK_THAT(roundTrip(k).epoch, WithinAbs(k.epoch, 1e-3));
    }
}

// Bug: keplerToTle's strL() (kepler_utils.cpp:14-32) truncates instead of rounding, so values whose
// binary representation sits just below the decimal one lose a unit in the last TLE digit
// (eccentricity 0.0006703 is written as 0006702, mean anomaly 325.0288 as 325.0287).
TEST_CASE("keplerToTle preserves elements to the precision of the TLE format", "[db][kepler_utils][!shouldfail]")
{
    auto k = issKepler();
    auto r = roundTrip(k);
    // Half a unit in the last place of each field
    CHECK_THAT(r.eccentricity, WithinAbs(k.eccentricity, 0.5e-7));
    CHECK_THAT(r.mean_anomaly, WithinAbs(k.mean_anomaly, 0.5e-4));
    CHECK_THAT(r.inclination, WithinAbs(k.inclination, 0.5e-4));
}

// Bug: keplerToTle's strLE() (kepler_utils.cpp:34-68) assumes the mantissa has exactly five
// significant digits. For a BSTAR of 1e-4 it writes "    1--1" (decoded as 1e-6) instead of
// " 10000-3".
TEST_CASE("keplerToTle encodes drag terms with few significant digits", "[db][kepler_utils][!shouldfail]")
{
    auto k = issKepler();
    k.bstar_drag_term = 0.0001;
    auto tle = satdump::keplerToTle(k);
    CHECK(tle.line1.substr(53, 8) == " 10000-3");
    CHECK_THAT(roundTrip(k).bstar_drag_term, WithinRel(0.0001, 1e-6));
}

// Bug: keplerToTle (kepler_utils.cpp:90) only converts designators of exactly 9 characters
// ("YYYY-NNNP"). Multi-letter piece codes such as "2023-001AB" are just cut to 8 characters,
// giving "2023-001" instead of "23001AB ".
TEST_CASE("keplerToTle converts designators with multi-letter piece codes", "[db][kepler_utils][!shouldfail]")
{
    auto k = issKepler();
    k.designator = "2023-001AB";
    auto tle = satdump::keplerToTle(k);
    CHECK(tle.line1.substr(9, 8) == "23001AB ");
}

// Bug: keplerToTle (kepler_utils.cpp:114) pads the epoch year with a space, not a zero, so years
// 2000-2009 come out as " 8" instead of "08" (fixed-column parsers that use atoi still cope).
TEST_CASE("keplerToTle writes a two digit zero padded epoch year", "[db][kepler_utils][!shouldfail]")
{
    auto tle = satdump::keplerToTle(issKepler());
    CHECK(tle.line1.substr(18, 2) == "08");
}
