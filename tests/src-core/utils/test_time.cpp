#include "utils/time.h"
#include <catch2/catch_test_macros.hpp>

using namespace satdump;

TEST_CASE("timestamp_to_string formats UTC timestamps", "[utils][time]")
{
    CHECK(timestamp_to_string(0) == "1970/01/01 00:00:00");
    CHECK(timestamp_to_string(1700000000) == "2023/11/14 22:13:20");
    CHECK(timestamp_to_string(951782400) == "2000/02/29 00:00:00");

    SECTION("fractional seconds are truncated") { CHECK(timestamp_to_string(1700000000.999) == "2023/11/14 22:13:20"); }

    SECTION("negative timestamps clamp to the epoch") { CHECK(timestamp_to_string(-100) == "1970/01/01 00:00:00"); }
}

TEST_CASE("timestamp_to_string appends a timezone for local time", "[utils][time]")
{
    std::string utc = timestamp_to_string(1700000000);
    std::string local = timestamp_to_string(1700000000, true);

    // Same "YYYY/MM/DD HH:MM:SS" prefix, followed by " <zone>"
    REQUIRE(local.size() > utc.size());
    CHECK(local[utc.size()] == ' ');
}

TEST_CASE("getTime returns a current, non-decreasing UNIX time", "[utils][time]")
{
    double a = getTime();
    double b = getTime();

    CHECK(a > 1.7e9); // After late 2023
    CHECK(b >= a);
}
