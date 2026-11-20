#include "proj_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using satdump_test_proj_standard::lonDiff;
using satdump_test_proj_standard::ScopedProj;

namespace
{
    nlohmann::json utmCfg(int zone, bool south) { return {{"type", "utm"}, {"zone", zone}, {"south", south}}; }
} // namespace

TEST_CASE("projection_tmerc UTM forward matches PROJ reference values", "[projection][standard][tmerc]")
{
    // References: proj +proj=utm +zone=.. [+south] +ellps=WGS84
    struct Case
    {
        int zone;
        bool south;
        double lon, lat, x, y;
    };
    const Case cases[] = {
        {32, false, 9, 45, 500000.000000, 4982950.400227}, // On the central meridian: 0.9996 * meridian arc
        {31, false, 3, 0, 500000.000000, 0.000000},        // Origin of zone 31
        {18, false, -75, 40, 500000.000000, 4427757.218738},
        {56, true, 151.2, -33.9, 333568.941012, 6247473.336844},
        {32, false, 3, 0, -168881.688527, 0.000000}, // Far from the central meridian
        {31, false, -75, 40, -5696676.566569, 8450903.099912},
    };

    for (const auto &c : cases)
    {
        ScopedProj sp(utmCfg(c.zone, c.south));
        REQUIRE(sp.setup());
        double x = 0, y = 0;
        REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, c.lon, c.lat, &x, &y));
        CHECK_THAT(x, WithinAbs(c.x, 1e-3));
        CHECK_THAT(y, WithinAbs(c.y, 1e-3));
    }
}

TEST_CASE("projection_tmerc UTM setup derives the zone parameters", "[projection][standard][tmerc]")
{
    ScopedProj sp(utmCfg(33, true));
    REQUIRE(sp.setup());
    CHECK(sp.p.params.zone == 33);
    CHECK_THAT(sp.p.lam0 * 180 / M_PI, WithinAbs(15, 1e-12)); // Central meridian of zone 33
    CHECK_THAT(sp.p.k0, WithinAbs(0.9996, 1e-15));
    CHECK_THAT(sp.p.x0, WithinAbs(500000, 1e-9));
    CHECK_THAT(sp.p.y0, WithinAbs(10000000, 1e-9)); // False northing of the southern hemisphere
}

TEST_CASE("projection_tmerc UTM without a zone picks the one containing lon0", "[projection][standard][tmerc]")
{
    struct Case
    {
        double lon0;
        int zone;
    };
    const Case cases[] = {{-177, 1}, {-179.9, 1}, {2, 31}, {9, 32}, {-75, 18}, {179.9, 60}};
    for (const auto &c : cases)
    {
        ScopedProj sp(nlohmann::json{{"type", "utm"}, {"lon0", c.lon0}});
        REQUIRE(sp.setup());
        CHECK(sp.p.params.zone == c.zone);
        CHECK_THAT(sp.p.lam0 * 180 / M_PI, WithinAbs(-183.0 + 6.0 * c.zone, 1e-12));
    }
}

TEST_CASE("projection_tmerc UTM forward/inverse round-trip across a zone", "[projection][standard][tmerc]")
{
    for (bool south : {false, true})
    {
        ScopedProj sp(utmCfg(17, south));
        REQUIRE(sp.setup());
        const double cm = -81;

        for (double lat = (south ? -80 : 0); lat <= (south ? 0 : 84); lat += 6)
        {
            for (double dlon = -9; dlon <= 9; dlon += 1.5)
            {
                double x, y;
                REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, cm + dlon, lat, &x, &y));
                double olon, olat;
                REQUIRE_FALSE(proj::projection_perform_inv(&sp.p, x, y, &olon, &olat));
                CHECK_THAT(olat, WithinAbs(lat, 1e-9));
                CHECK_THAT(lonDiff(olon, cm + dlon), WithinAbs(0, 1e-9));
            }
        }
    }
}

TEST_CASE("projection_tmerc UTM image config puts the false origin at the image center", "[projection][standard][tmerc]")
{
    // proj_ui.h style config: scale meters/pixel, image center at easting 0 / northing offset_y
    nlohmann::json cfg = {{"type", "utm"}, {"zone", 31}, {"south", false}, {"scale", 1000.0}, {"width", 2000}, {"height", 1000}};
    ScopedProj sp(cfg);
    REQUIRE(sp.setup());

    double x, y;
    REQUIRE_FALSE(proj::projection_perform_fwd(&sp.p, 3, 0, &x, &y));
    // Easting 500000 with offset_x = -width/2*scale => x = (500000 + 1e6) / 1000
    CHECK_THAT(x, WithinAbs(1500, 1e-6));
    CHECK_THAT(y, WithinAbs(500, 1e-6));

    double lon, lat;
    REQUIRE_FALSE(proj::projection_perform_inv(&sp.p, x, y, &lon, &lat));
    CHECK_THAT(lon, WithinAbs(3, 1e-9));
    CHECK_THAT(lat, WithinAbs(0, 1e-9));
}
