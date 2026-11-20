#include "projection/tps/latlontps_proj.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using satdump::proj::LatLonTpsProjHelper;
using satdump::projection::GCP;

namespace
{
    // Regular grid where pixel (x, y) is at lon = lon0 + 0.5x, lat = lat0 - 0.5y
    std::vector<GCP> gridGcps(double lon0, double lat0)
    {
        std::vector<GCP> g;
        for (int y = 0; y <= 20; y += 5)
            for (int x = 0; x <= 20; x += 5)
                g.push_back({(double)x, (double)y, lon0 + 0.5 * x, lat0 - 0.5 * y});
        return g;
    }

    double lonDiff(double a, double b)
    {
        double d = std::fmod(a - b, 360.0);
        if (d > 180)
            d -= 360;
        if (d < -180)
            d += 360;
        return d;
    }
} // namespace

TEST_CASE("LatLonTpsProjHelper forward reproduces the GCPs", "[projection][tps][latlontps]")
{
    auto gcps = gridGcps(30, 20);
    LatLonTpsProjHelper h(gcps, true, false);
    for (auto &g : gcps)
    {
        double x, y;
        h.forward(geodetic::geodetic_coords_t(g.lat, g.lon, 0), x, y);
        CHECK_THAT(x, WithinAbs(g.x, 1e-6));
        CHECK_THAT(y, WithinAbs(g.y, 1e-6));
    }

    // Lat/lon is affine in pixels here, so in-between points are exact too
    double x, y;
    h.forward(geodetic::geodetic_coords_t(20 - 0.5 * 7.5, 30 + 0.5 * 12.5, 0), x, y);
    CHECK_THAT(x, WithinAbs(12.5, 1e-6));
    CHECK_THAT(y, WithinAbs(7.5, 1e-6));
}

TEST_CASE("LatLonTpsProjHelper forward handles a grid straddling the antimeridian", "[projection][tps][latlontps]")
{
    // lon runs 175 .. 185 (= -175), centered on 180
    auto gcps = gridGcps(175, -10);
    for (auto &g : gcps)
        if (g.lon > 180)
            g.lon -= 360;
    LatLonTpsProjHelper h(gcps, true, false);

    for (auto &g : gcps)
    {
        double x, y;
        h.forward(geodetic::geodetic_coords_t(g.lat, g.lon, 0), x, y);
        CHECK_THAT(x, WithinAbs(g.x, 1e-6));
        CHECK_THAT(y, WithinAbs(g.y, 1e-6));
    }

    double x, y;
    h.forward(geodetic::geodetic_coords_t(-12, -179, 0), x, y); // lon 181 => x = 12, lat -12 => y = 4
    CHECK_THAT(x, WithinAbs(12, 1e-6));
    CHECK_THAT(y, WithinAbs(4, 1e-6));
}

TEST_CASE("LatLonTpsProjHelper forward handles GCPs over a pole", "[projection][tps][latlontps]")
{
    // A small cap around the north pole, sampled on rings
    std::vector<GCP> gcps;
    int i = 0;
    for (double lat : {85.0, 87.0, 89.0, 89.95})
        for (double lon = -180; lon < 180; lon += 45)
            gcps.push_back({(double)(i % 8), (double)(i++ / 8), lon, lat});
    LatLonTpsProjHelper h(gcps, true, false);

    for (auto &g : gcps)
    {
        double x, y;
        h.forward(geodetic::geodetic_coords_t(g.lat, g.lon, 0), x, y);
        CHECK_THAT(x, WithinAbs(g.x, 1e-5));
        CHECK_THAT(y, WithinAbs(g.y, 1e-5));
    }
}

// Bug: latlontps_proj.cpp:186-187 reads the reverse TPS output as {lat, lon}, but the
// reverse spline is built with values {lon, lat} (initTPSTransformREV, afXY), so
// reverse() returns latitude and longitude swapped (and the longitude unshift is
// applied to the latitude).
TEST_CASE("LatLonTpsProjHelper reverse reproduces the GCPs", "[projection][tps][latlontps][!shouldfail]")
{
    auto gcps = gridGcps(30, 20);
    LatLonTpsProjHelper h(gcps, false, true);
    for (auto &g : gcps)
    {
        geodetic::geodetic_coords_t pos;
        h.reverse(g.x, g.y, pos);
        CHECK_THAT(pos.lat, WithinAbs(g.lat, 1e-6));
        CHECK_THAT(lonDiff(pos.lon, g.lon), WithinAbs(0, 1e-6));
    }
}
