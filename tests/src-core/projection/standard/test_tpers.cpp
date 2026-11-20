#include "projection/standard/proj_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using proj_test::lonDiff;
using proj_test::ScopedProj;

namespace
{
    nlohmann::json tpersCfg(double alt, double lon0, double lat0, double tilt = 0, double azi = 0)
    {
        return {{"type", "tpers"}, {"altitude", alt}, {"lon0", lon0}, {"lat0", lat0}, {"tilt", tilt}, {"azimuth", azi}};
    }

    double centralAngle(double lat1, double lon1, double lat2, double lon2)
    {
        const double d = M_PI / 180;
        double c = std::sin(lat1 * d) * std::sin(lat2 * d) + std::cos(lat1 * d) * std::cos(lat2 * d) * std::cos((lon2 - lon1) * d);
        return std::acos(std::min(1.0, std::max(-1.0, c))) / d;
    }
} // namespace

TEST_CASE("projection_tpers forward matches PROJ reference values", "[projection][standard][tpers]")
{
    // TPERS is spherical; references from PROJ with +R=6378137 (the WGS84 semi-major axis)
    struct Case
    {
        double alt, lon0, lat0, tilt, azi;
        double lon, lat, x, y;
    };
    const Case cases[] = {
        // Equatorial aspect
        {30000000, 0, 0, 0, 0, 10, 20, 1024512.845784, 2147400.482394},
        {30000000, 0, 0, 0, 0, -30, -40, -2279824.568701, -3825999.909493},
        {30000000, 0, 0, 0, 0, 0, 0, 0, 0},
        // Oblique aspect with tilt and azimuth
        {10000000, 0, 45, 20, 30, 10, 60, -333474.352516, 1697615.894664},
        {10000000, 0, 45, 20, 30, -20, 30, -960853.614412, -2390055.418387},
        // North polar aspect
        {10000000, 0, 90, 0, 0, 10, 80, 190478.658830, -1080258.154847},
    };

    for (const auto &c : cases)
    {
        ScopedProj sp(tpersCfg(c.alt, c.lon0, c.lat0, c.tilt, c.azi));
        REQUIRE(sp.setup());
        double x = 0, y = 0;
        REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, c.lon, c.lat, &x, &y));
        CHECK_THAT(x, WithinAbs(c.x, 1e-2));
        CHECK_THAT(y, WithinAbs(c.y, 1e-2));
    }
}

TEST_CASE("projection_tpers reports the far side of the Earth as invalid", "[projection][standard][tpers]")
{
    // From 30000 km the horizon is acos(R / (R + h)) ~ 79.9 degrees from the view center
    ScopedProj sp(tpersCfg(30000000, 20, 10));
    REQUIRE(sp.setup());

    double x, y;
    CHECK_FALSE(proj::projection_perform_fwd(&sp.p, 20, 10, &x, &y));
    CHECK_FALSE(proj::projection_perform_fwd(&sp.p, 90, 10, &x, &y));
    CHECK(proj::projection_perform_fwd(&sp.p, 200, -10, &x, &y)); // Antipode
    CHECK(proj::projection_perform_fwd(&sp.p, 110, 10, &x, &y));  // 90 degrees away
    CHECK(proj::projection_perform_fwd(&sp.p, 20, -80, &x, &y));  // 90 degrees south
}

TEST_CASE("projection_tpers inverse rejects points outside the visible disk", "[projection][standard][tpers]")
{
    ScopedProj sp(tpersCfg(30000000, 0, 0));
    REQUIRE(sp.setup());

    // Disk radius in projected meters is R * sqrt(h / (h + 2R)) ~ 5.30e6 for h = 30000 km
    double lon, lat;
    CHECK_FALSE(proj::projection_perform_inv(&sp.p, 5.2e6, 0, &lon, &lat));
    CHECK(proj::projection_perform_inv(&sp.p, 5.4e6, 0, &lon, &lat));
    CHECK(proj::projection_perform_inv(&sp.p, 0, 5.4e6, &lon, &lat));

    // The view center maps back to the projection center
    REQUIRE_FALSE(proj::projection_perform_inv(&sp.p, 0, 0, &lon, &lat));
    CHECK_THAT(lon, WithinAbs(0, 1e-12));
    CHECK_THAT(lat, WithinAbs(0, 1e-12));
}

TEST_CASE("projection_tpers forward/inverse round-trip in every aspect", "[projection][standard][tpers]")
{
    struct Aspect
    {
        double lon0, lat0, tilt, azi;
    };
    const Aspect aspects[] = {
        {0, 0, 0, 0},     // Equatorial
        {-60, 35, 0, 0},  // Oblique
        {120, 90, 0, 0},  // North pole
        {0, -90, 0, 0},   // South pole
        {15, 50, 25, 40}, // Tilted
    };

    for (const auto &a : aspects)
    {
        ScopedProj sp(tpersCfg(20000000, a.lon0, a.lat0, a.tilt, a.azi));
        REQUIRE(sp.setup());

        for (double lat = -85; lat <= 85; lat += 10)
        {
            for (double lon = -175; lon <= 175; lon += 15)
            {
                // Keep well inside the horizon (~75.6 degrees for 20000 km), less for the tilted view
                if (centralAngle(lat, lon, a.lat0, a.lon0) > (a.tilt != 0 ? 45 : 65))
                    continue;

                double x, y;
                REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, lon, lat, &x, &y));
                double olon, olat;
                REQUIRE_FALSE(proj::projection_perform_inv(&sp.p, x, y, &olon, &olat));
                CHECK_THAT(olat, WithinAbs(lat, 1e-7));
                CHECK_THAT(lonDiff(olon, lon), WithinAbs(0, 1e-7));
            }
        }
    }
}

TEST_CASE("projection_tpers image config centers the view on the middle pixel", "[projection][standard][tpers]")
{
    // center_lon/center_lat/scale/width/height are expanded by proj_json.h into lon0/lat0/scalars/offsets
    nlohmann::json cfg = {{"type", "tpers"}, {"altitude", 30000000.0}, {"center_lon", 10.0}, {"center_lat", 40.0}, {"tilt", 0.0}, {"azimuth", 0.0}, {"scale", 8000.0}, {"width", 800}, {"height", 600}};
    ScopedProj sp(cfg);
    REQUIRE(sp.setup());

    double x, y;
    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, 10, 40, &x, &y));
    CHECK_THAT(x, WithinAbs(400, 1e-6));
    CHECK_THAT(y, WithinAbs(300, 1e-6));

    // Image rows grow southwards, columns eastwards
    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, 10, 45, &x, &y));
    CHECK(y < 300);
    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, 15, 40, &x, &y));
    CHECK(x > 400);

    double lon, lat;
    REQUIRE_FALSE(proj::projection_perform_inv(&sp.p, 400, 300, &lon, &lat));
    CHECK_THAT(lon, WithinAbs(10, 1e-9));
    CHECK_THAT(lat, WithinAbs(40, 1e-9));
}

TEST_CASE("projection_tpers setup fails without a positive altitude", "[projection][standard][tpers]")
{
    ScopedProj sp(tpersCfg(0, 0, 0));
    CHECK_FALSE(sp.setup());
}
