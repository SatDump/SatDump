#include "proj_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using satdump_test_proj_standard::lonDiff;
using satdump_test_proj_standard::ScopedProj;

namespace
{
    const double GEO_ALT_M = 35786023.0;

    nlohmann::json geosCfg(double lon0, bool sweep_x = false)
    {
        return {{"type", "geos"}, {"altitude", GEO_ALT_M}, {"sweep_x", sweep_x}, {"lon0", lon0}};
    }

    // Great-circle angle from (0,0) on a sphere, in degrees
    double centralAngle(double lat, double lon) { return std::acos(std::cos(lat * M_PI / 180) * std::cos(lon * M_PI / 180)) * 180 / M_PI; }
} // namespace

TEST_CASE("projection_geos sub-satellite point maps to the origin", "[projection][standard][geos]")
{
    for (double lon0 : {0.0, -75.0, 140.7})
    {
        ScopedProj sp(geosCfg(lon0));
        REQUIRE(sp.setup());

        double x = 1, y = 1;
        REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, lon0, 0, &x, &y));
        CHECK_THAT(x, WithinAbs(0, 1e-6));
        CHECK_THAT(y, WithinAbs(0, 1e-6));

        double lon = 1, lat = 1;
        REQUIRE_FALSE(proj::projection_perform_inv(&sp.p, 0, 0, &lon, &lat));
        CHECK_THAT(lon, WithinAbs(lon0, 1e-9));
        CHECK_THAT(lat, WithinAbs(0, 1e-9));
    }
}

TEST_CASE("projection_geos forward matches PROJ reference values", "[projection][standard][geos]")
{
    // References: proj +proj=geos +h=35786023 +ellps=WGS84 [+lon_0=..] [+sweep=x]
    struct Case
    {
        double lon0;
        bool sweep_x;
        double lon, lat, x, y;
    };
    const Case cases[] = {
        {0, false, 10, 20, 1027290.667226, 2135974.472853},
        {0, false, -30, -40, -2305067.069513, -3825174.002907},
        {0, false, 60, 50, 3165702.297646, 4300750.912042},
        {0, true, 10, 20, 1025460.803908, 2136852.771909},
        {0, true, -30, -40, -2291893.253106, -3833062.413400},
        {-75, false, -70, 20, 516686.102671, 2140651.713622},
        {-75, false, -80, -10, -545626.664990, -1096095.954098},
    };

    for (const auto &c : cases)
    {
        ScopedProj sp(geosCfg(c.lon0, c.sweep_x));
        REQUIRE(sp.setup());
        double x = 0, y = 0;
        REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, c.lon, c.lat, &x, &y));
        CHECK_THAT(x, WithinAbs(c.x, 1e-2));
        CHECK_THAT(y, WithinAbs(c.y, 1e-2));
    }
}

TEST_CASE("projection_geos forward is symmetric about the sub-satellite meridian and equator", "[projection][standard][geos]")
{
    ScopedProj sp(geosCfg(0));
    REQUIRE(sp.setup());

    double x1, y1, x2, y2, x3, y3;
    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, 25, 35, &x1, &y1));
    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, -25, 35, &x2, &y2));
    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, 25, -35, &x3, &y3));
    CHECK_THAT(x1, WithinAbs(-x2, 1e-6));
    CHECK_THAT(y1, WithinAbs(y2, 1e-6));
    CHECK_THAT(x1, WithinAbs(x3, 1e-6));
    CHECK_THAT(y1, WithinAbs(-y3, 1e-6));
}

TEST_CASE("projection_geos reports points beyond the Earth limb as invalid", "[projection][standard][geos]")
{
    ScopedProj sp(geosCfg(0));
    REQUIRE(sp.setup());

    double x, y;
    // From GEO the visible cap extends ~81.3 degrees from the sub-satellite point
    CHECK_FALSE(proj::projection_perform_fwd(&sp.p, 80, 0, &x, &y));
    CHECK_FALSE(proj::projection_perform_fwd(&sp.p, 0, 79, &x, &y));
    CHECK(proj::projection_perform_fwd(&sp.p, 83, 0, &x, &y));
    CHECK(proj::projection_perform_fwd(&sp.p, 0, 84, &x, &y));
    CHECK(proj::projection_perform_fwd(&sp.p, 180, 0, &x, &y));
    CHECK(proj::projection_perform_fwd(&sp.p, -120, 30, &x, &y));
    CHECK(proj::projection_perform_fwd(&sp.p, 0, 90, &x, &y));
}

TEST_CASE("projection_geos inverse rejects scan angles that miss the Earth", "[projection][standard][geos]")
{
    ScopedProj sp(geosCfg(0));
    REQUIRE(sp.setup());

    // The Earth disk radius in projection meters is about h * asin(a / (a + h)) ~ 5.43e6
    double lon, lat;
    CHECK_FALSE(proj::projection_perform_inv(&sp.p, 5.2e6, 0, &lon, &lat));
    CHECK(proj::projection_perform_inv(&sp.p, 5.6e6, 0, &lon, &lat));
    CHECK(proj::projection_perform_inv(&sp.p, 0, -5.6e6, &lon, &lat));
    CHECK(proj::projection_perform_inv(&sp.p, 4.5e6, 4.5e6, &lon, &lat));
}

TEST_CASE("projection_geos forward/inverse round-trip over the visible disk", "[projection][standard][geos]")
{
    for (bool sweep_x : {false, true})
    {
        for (double lon0 : {0.0, 140.7, -137.2})
        {
            ScopedProj sp(geosCfg(lon0, sweep_x));
            REQUIRE(sp.setup());

            for (double lat = -70; lat <= 70; lat += 10)
            {
                for (double dlon = -70; dlon <= 70; dlon += 10)
                {
                    if (centralAngle(lat, dlon) > 75)
                        continue;

                    double lon = lon0 + dlon;
                    double x, y;
                    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, lon, lat, &x, &y));
                    double olon, olat;
                    REQUIRE_FALSE(proj::projection_perform_inv(&sp.p, x, y, &olon, &olat));
                    CHECK_THAT(olat, WithinAbs(lat, 1e-8));
                    CHECK_THAT(lonDiff(olon, lon), WithinAbs(0, 1e-8));
                    // Inverse output is always normalised into [-180, 180]
                    CHECK(olon >= -180);
                    CHECK(olon <= 180);
                }
            }
        }
    }
}

TEST_CASE("projection_geos applies pixel scalars and offsets", "[projection][standard][geos]")
{
    // 2km pixels on a 5424 pixel full disk, like GOES ABI, with y pointing down
    nlohmann::json cfg = geosCfg(-75);
    cfg["scalar_x"] = 2000.0;
    cfg["scalar_y"] = -2000.0;
    cfg["offset_x"] = -5424 * 0.5 * 2000.0;
    cfg["offset_y"] = 5424 * 0.5 * 2000.0;
    ScopedProj sp(cfg);
    REQUIRE(sp.setup());

    double x, y;
    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, -75, 0, &x, &y));
    CHECK_THAT(x, WithinAbs(2712, 1e-6));
    CHECK_THAT(y, WithinAbs(2712, 1e-6));

    // North of the sub-satellite point is a smaller row index
    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, -75, 30, &x, &y));
    CHECK_THAT(x, WithinAbs(2712, 1e-6));
    CHECK(y < 2712);

    double lon, lat;
    REQUIRE_FALSE(proj::projection_perform_inv(&sp.p, x, y, &lon, &lat));
    CHECK_THAT(lon, WithinAbs(-75, 1e-8));
    CHECK_THAT(lat, WithinAbs(30, 1e-8));
}

TEST_CASE("projection_geos setup fails without a positive altitude", "[projection][standard][geos]")
{
    ScopedProj sp(nlohmann::json{{"type", "geos"}});
    CHECK_FALSE(sp.setup());

    ScopedProj sp2(nlohmann::json{{"type", "geos"}, {"altitude", -5.0}});
    CHECK_FALSE(sp2.setup());
}
