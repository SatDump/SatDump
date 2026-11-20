#include "utils/unit_parser.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace satdump;
using Catch::Approx;

TEST_CASE("parseUnitFromString parses frequencies", "[utils][unit_parser]")
{
    double out = 0;

    REQUIRE(parseUnitFromString("100 MHz", out, UNIT_HERTZ));
    CHECK(out == Approx(100e6));

    REQUIRE(parseUnitFromString("2.4GHz", out, UNIT_HERTZ));
    CHECK(out == Approx(2.4e9));

    REQUIRE(parseUnitFromString("137.1e3 kHz", out, UNIT_HERTZ));
    CHECK(out == Approx(137.1e6));

    REQUIRE(parseUnitFromString("50 Hz", out, UNIT_HERTZ));
    CHECK(out == Approx(50));
}

TEST_CASE("parseUnitFromString distinguishes prefixes by case and length", "[utils][unit_parser]")
{
    double out = 0;

    REQUIRE(parseUnitFromString("10 mHz", out, UNIT_HERTZ));
    CHECK(out == Approx(10e-3));

    // "daHz" must not be matched as "Hz"
    REQUIRE(parseUnitFromString("3 daHz", out, UNIT_HERTZ));
    CHECK(out == Approx(30));
}

TEST_CASE("parseUnitFromString parses lengths", "[utils][unit_parser]")
{
    double out = 0;

    REQUIRE(parseUnitFromString("5 km", out, UNIT_METER));
    CHECK(out == Approx(5000));

    REQUIRE(parseUnitFromString("11.5 um", out, UNIT_METER));
    CHECK(out == Approx(11.5e-6));

    REQUIRE(parseUnitFromString("2 m", out, UNIT_METER));
    CHECK(out == Approx(2));
}

TEST_CASE("parseUnitFromString rejects strings without a known unit", "[utils][unit_parser]")
{
    double out = 42;

    CHECK_FALSE(parseUnitFromString("100", out, UNIT_HERTZ));
    CHECK_FALSE(parseUnitFromString("100 V", out, UNIT_HERTZ));
    CHECK_FALSE(parseUnitFromString("", out, UNIT_HERTZ));
    CHECK(out == 42);
}
