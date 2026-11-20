#include "proj_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using satdump_test_proj_standard::ScopedProj;

namespace
{
    // Spherical Web Mercator on the WGS84 semi-major axis: x = a*lon, y = a*ln(tan(pi/4 + lat/2))
    void webmercRef(double lon_deg, double lat_deg, double &x, double &y)
    {
        const double a = 6378137.0;
        x = a * lon_deg * M_PI / 180.0;
        y = a * std::log(std::tan(M_PI / 4 + lat_deg * M_PI / 360.0));
    }
} // namespace

TEST_CASE("projection_webmerc forward matches the spherical Web Mercator formula", "[projection][standard][webmerc]")
{
    ScopedProj sp;
    sp.p.type = proj::ProjType_WebMerc;
    REQUIRE(sp.setup());

    double x = 1, y = 1;
    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, 0, 0, &x, &y));
    CHECK_THAT(x, WithinAbs(0, 1e-6));
    CHECK_THAT(y, WithinAbs(0, 1e-6));

    // Reference values from PROJ: echo "10 20" | proj +proj=webmerc +ellps=WGS84
    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, 10, 20, &x, &y));
    CHECK_THAT(x, WithinAbs(1113194.907933, 1e-3));
    CHECK_THAT(y, WithinAbs(2273030.926988, 1e-3));
    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, -170, -80, &x, &y));
    CHECK_THAT(x, WithinAbs(-18924313.434857, 1e-3));
    CHECK_THAT(y, WithinAbs(-15538711.096309, 1e-3));

    for (double lat = -84; lat <= 84; lat += 12)
    {
        for (double lon = -175; lon <= 175; lon += 35)
        {
            double rx, ry;
            webmercRef(lon, lat, rx, ry);
            REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, lon, lat, &x, &y));
            CHECK_THAT(x, WithinAbs(rx, 1e-6));
            CHECK_THAT(y, WithinAbs(ry, 1e-6));
        }
    }
}

TEST_CASE("projection_webmerc y is antisymmetric in latitude and monotonic", "[projection][standard][webmerc]")
{
    ScopedProj sp;
    sp.p.type = proj::ProjType_WebMerc;
    REQUIRE(sp.setup());

    double prev_y = -1e30;
    for (double lat = -85; lat <= 85; lat += 5)
    {
        double x1, y1, x2, y2;
        REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, 30, lat, &x1, &y1));
        REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, 30, -lat, &x2, &y2));
        CHECK_THAT(y1, WithinAbs(-y2, 1e-6));
        CHECK(y1 > prev_y);
        prev_y = y1;
    }
}

// Bug: webmerc.cpp:37 projection_webmerc_inv always returns true (error), so
// projection_perform_inv never converts the result and every Web Mercator inverse fails.
TEST_CASE("projection_webmerc inverse round-trips the forward projection", "[projection][standard][webmerc][!shouldfail]")
{
    ScopedProj sp;
    sp.p.type = proj::ProjType_WebMerc;
    REQUIRE(sp.setup());

    double x, y;
    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, 10, 20, &x, &y));
    double lon = 0, lat = 0;
    REQUIRE_FALSE(proj::projection_perform_inv(&sp.p, x, y, &lon, &lat));
    CHECK_THAT(lon, WithinAbs(10, 1e-9));
    CHECK_THAT(lat, WithinAbs(20, 1e-9));
}
