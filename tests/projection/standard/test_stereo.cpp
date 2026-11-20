#include "proj_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using satdump_test_proj_standard::lonDiff;
using satdump_test_proj_standard::ScopedProj;

namespace
{
    nlohmann::json stereoCfg(double lon0, double lat0) { return {{"type", "stereo"}, {"lon0", lon0}, {"lat0", lat0}}; }
} // namespace

TEST_CASE("projection_stereo forward matches PROJ reference values", "[projection][standard][stereo]")
{
    // References: proj +proj=stere +lat_0=.. +lon_0=0 +ellps=WGS84 (k0 = 1)
    struct Case
    {
        double lat0, lon, lat, x, y;
    };
    const Case cases[] = {
        {90, 0, 60, 0, -3426439.353492},
        {90, 45, 70, 1595180.092288, -1595180.092288},
        {90, -120, 80, -969661.929350, 559834.575933},
        {-90, 0, -60, 0, 3426439.353492},
        {-90, 45, -70, 1595180.092288, 1595180.092288},
        {45, 5, 50, 359125.092995, 567873.250272},
        {45, -10, 30, -980398.432162, -1621205.988783},
        {0, 10, 10, 1107535.219065, 1117092.348128},
    };

    for (const auto &c : cases)
    {
        ScopedProj sp(stereoCfg(0, c.lat0));
        REQUIRE(sp.setup());
        double x = 0, y = 0;
        REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, c.lon, c.lat, &x, &y));
        CHECK_THAT(x, WithinAbs(c.x, 1e-2));
        CHECK_THAT(y, WithinAbs(c.y, 1e-2));
    }
}

TEST_CASE("projection_stereo maps the projection center to the origin", "[projection][standard][stereo]")
{
    for (double lat0 : {90.0, -90.0, 45.0, 0.0, -30.0})
    {
        ScopedProj sp(stereoCfg(25, lat0));
        REQUIRE(sp.setup());
        double x = 1, y = 1;
        REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, 25, lat0, &x, &y));
        CHECK_THAT(x, WithinAbs(0, 1e-6));
        CHECK_THAT(y, WithinAbs(0, 1e-6));

        double lon, lat;
        REQUIRE_FALSE(proj::projection_perform_inv(&sp.p, 0, 0, &lon, &lat));
        CHECK_THAT(lat, WithinAbs(lat0, 1e-9));
        if (std::fabs(lat0) != 90)
            CHECK_THAT(lon, WithinAbs(25, 1e-9));
    }
}

TEST_CASE("projection_stereo polar aspect is rotationally symmetric", "[projection][standard][stereo]")
{
    ScopedProj sp(stereoCfg(0, 90));
    REQUIRE(sp.setup());

    // Every point on a parallel is at the same distance from the pole
    double x0, y0;
    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, 0, 65, &x0, &y0));
    const double r0 = std::hypot(x0, y0);
    for (double lon = -150; lon < 180; lon += 30)
    {
        double x, y;
        REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, lon, 65, &x, &y));
        CHECK_THAT(std::hypot(x, y), WithinAbs(r0, 1e-6));
        // Meridian lon is drawn at angle lon from the -y axis (PROJ convention, +x towards 90E)
        CHECK_THAT(std::atan2(x, -y) * 180 / M_PI, WithinAbs(lon, 1e-9));
    }
}

TEST_CASE("projection_stereo forward/inverse round-trip in every aspect", "[projection][standard][stereo]")
{
    struct Aspect
    {
        double lon0, lat0, lat_min, lat_max;
    };
    const Aspect aspects[] = {
        {0, 90, 0, 89},
        {-45, -90, -89, 0},
        {10, 50, -20, 89},
        {-100, 0, -70, 70},
    };

    for (const auto &a : aspects)
    {
        ScopedProj sp(stereoCfg(a.lon0, a.lat0));
        REQUIRE(sp.setup());

        for (double lat = a.lat_min; lat <= a.lat_max; lat += 7)
        {
            for (double dlon = -80; dlon <= 80; dlon += 20)
            {
                double lon = a.lon0 + dlon;
                double x, y;
                REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, lon, lat, &x, &y));
                double olon, olat;
                REQUIRE_FALSE(proj::projection_perform_inv(&sp.p, x, y, &olon, &olat));
                // The inverse latitude iteration converges to 1e-10 rad
                CHECK_THAT(olat, WithinAbs(lat, 1e-7));
                CHECK_THAT(lonDiff(olon, lon), WithinAbs(0, 1e-7));
            }
        }
    }
}

TEST_CASE("projection_stereo image config centers the view on the middle pixel", "[projection][standard][stereo]")
{
    nlohmann::json cfg = {{"type", "stereo"}, {"center_lon", -30.0}, {"center_lat", 60.0}, {"scale", 2000.0}, {"width", 1000}, {"height", 500}};
    ScopedProj sp(cfg);
    REQUIRE(sp.setup());

    double x, y;
    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, -30, 60, &x, &y));
    CHECK_THAT(x, WithinAbs(500, 1e-6));
    CHECK_THAT(y, WithinAbs(250, 1e-6));

    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, -30, 62, &x, &y));
    CHECK(y < 250);

    double lon, lat;
    REQUIRE_FALSE(proj::projection_perform_inv(&sp.p, 500, 250, &lon, &lat));
    CHECK_THAT(lon, WithinAbs(-30, 1e-9));
    CHECK_THAT(lat, WithinAbs(60, 1e-9));
}
