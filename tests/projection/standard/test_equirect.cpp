#include "proj_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using satdump_test_proj_standard::ScopedProj;

namespace
{
    // Full-globe 360x180 image: pixel (0,0) is the top-left corner at 180W 90N, 1 degree per pixel
    nlohmann::json globeImageCfg()
    {
        return {{"type", "equirec"}, {"offset_x", -180.0}, {"offset_y", 90.0}, {"scalar_x", 1.0}, {"scalar_y", -1.0}};
    }
} // namespace

TEST_CASE("projection_equirect forward without offsets returns plain degrees", "[projection][standard][equirect]")
{
    ScopedProj sp;
    sp.p.type = proj::ProjType_Equirectangular;
    REQUIRE(sp.setup());

    double x = 0, y = 0;
    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, 0, 0, &x, &y));
    CHECK_THAT(x, WithinAbs(0, 1e-12));
    CHECK_THAT(y, WithinAbs(0, 1e-12));

    // Unlike every other projection, equirect is NOT scaled by the ellipsoid radius
    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, 12.5, -33.25, &x, &y));
    CHECK_THAT(x, WithinAbs(12.5, 1e-12));
    CHECK_THAT(y, WithinAbs(-33.25, 1e-12));

    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, -180, 90, &x, &y));
    CHECK_THAT(x, WithinAbs(-180, 1e-12));
    CHECK_THAT(y, WithinAbs(90, 1e-12));
}

TEST_CASE("projection_equirect maps a full-globe image config to pixel coordinates", "[projection][standard][equirect]")
{
    ScopedProj sp(globeImageCfg());
    REQUIRE(sp.setup());

    struct Case
    {
        double lon, lat, x, y;
    };
    const Case cases[] = {
        {-180, 90, 0, 0},
        {0, 0, 180, 90},
        {180, -90, 360, 180},
        {-90.5, 45.25, 89.5, 44.75},
        {120, -30, 300, 120},
    };

    for (const auto &c : cases)
    {
        double x = 0, y = 0;
        REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, c.lon, c.lat, &x, &y));
        CHECK_THAT(x, WithinAbs(c.x, 1e-9));
        CHECK_THAT(y, WithinAbs(c.y, 1e-9));

        double lon = 0, lat = 0;
        REQUIRE_FALSE(proj::projection_perform_inv(&sp.p, c.x, c.y, &lon, &lat));
        CHECK_THAT(lat, WithinAbs(c.lat, 1e-9));
        // +180 is normalised to the same meridian, either sign is fine
        CHECK_THAT(satdump_test_proj_standard::lonDiff(lon, c.lon), WithinAbs(0, 1e-9));
    }
}

TEST_CASE("projection_equirect inverse wraps longitudes past the antimeridian", "[projection][standard][equirect]")
{
    ScopedProj sp;
    sp.p.type = proj::ProjType_Equirectangular;
    REQUIRE(sp.setup());

    double lon = 0, lat = 0;
    REQUIRE_FALSE(proj::projection_perform_inv(&sp.p, 190, 10, &lon, &lat));
    CHECK_THAT(lon, WithinAbs(-170, 1e-9));
    CHECK_THAT(lat, WithinAbs(10, 1e-9));

    REQUIRE_FALSE(proj::projection_perform_inv(&sp.p, -200, -10, &lon, &lat));
    CHECK_THAT(lon, WithinAbs(160, 1e-9));

    REQUIRE_FALSE(proj::projection_perform_inv(&sp.p, 540, 0, &lon, &lat));
    CHECK_THAT(std::fabs(lon), WithinAbs(180, 1e-9));
}

TEST_CASE("projection_equirect central meridian shifts the inverse output", "[projection][standard][equirect]")
{
    ScopedProj sp(nlohmann::json{{"type", "equirec"}, {"lon0", 170.0}});
    REQUIRE(sp.setup());

    double lon = 0, lat = 0;
    REQUIRE_FALSE(proj::projection_perform_inv(&sp.p, 0, 0, &lon, &lat));
    CHECK_THAT(lon, WithinAbs(170, 1e-9));

    // 20 degrees east of a 170E central meridian is 170W
    REQUIRE_FALSE(proj::projection_perform_inv(&sp.p, 20, 0, &lon, &lat));
    CHECK_THAT(lon, WithinAbs(-170, 1e-9));

    double x = 0, y = 0;
    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, 175, 5, &x, &y));
    CHECK_THAT(x, WithinAbs(5, 1e-9));
    CHECK_THAT(y, WithinAbs(5, 1e-9));
}

// Bug: proj.cpp:60 subtracts lam0 in projection_perform_fwd but never normalises the
// result, while projection_perform_inv (proj.cpp:166-169) does. With lon0 = 170 the inverse
// maps x = 20 to 170W, but the forward maps 170W to x = -340 instead of x = 20, so the
// forward map is not the inverse of the inverse map across the antimeridian.
TEST_CASE("projection_equirect forward wraps longitude relative to the central meridian", "[projection][standard][equirect][!shouldfail]")
{
    ScopedProj sp(nlohmann::json{{"type", "equirec"}, {"lon0", 170.0}});
    REQUIRE(sp.setup());

    double x = 0, y = 0;
    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, -170, 0, &x, &y));
    CHECK_THAT(x, WithinAbs(20, 1e-9));
}

TEST_CASE("projection_setup rejects an invalid projection type", "[projection][standard]")
{
    ScopedProj sp;
    CHECK(sp.p.type == proj::ProjType_Invalid);
    CHECK_FALSE(sp.setup());
}

TEST_CASE("projection_setup loads the WGS84 ellipsoid in meters", "[projection][standard]")
{
    ScopedProj sp;
    sp.p.type = proj::ProjType_Equirectangular;
    REQUIRE(sp.setup());
    CHECK_THAT(sp.p.a, WithinAbs(6378137.0, 1e-6));
    // WGS84 first eccentricity squared
    CHECK_THAT(sp.p.es, WithinAbs(0.00669437999014, 1e-12));
    CHECK_THAT(sp.p.e, WithinAbs(std::sqrt(0.00669437999014), 1e-10));
    CHECK_THAT(sp.p.one_es, WithinAbs(1 - 0.00669437999014, 1e-12));
}
