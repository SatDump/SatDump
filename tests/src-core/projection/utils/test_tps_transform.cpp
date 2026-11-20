#include "projection/utils/tps_transform.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

using Catch::Matchers::WithinAbs;
using satdump::projection::GCP;
using satdump::projection::TPSTransform;

namespace
{
    // Pixel (x, y) <=> lon = -10 + 0.2x + 0.01y, lat = 50 - 0.1y (affine, slightly sheared)
    GCP affineGcp(double x, double y) { return {x, y, -10 + 0.2 * x + 0.01 * y, 50 - 0.1 * y}; }

    std::vector<GCP> affineGrid()
    {
        std::vector<GCP> g;
        for (int y = 0; y <= 100; y += 25)
            for (int x = 0; x <= 100; x += 20)
                g.push_back(affineGcp(x, y));
        return g;
    }
} // namespace

TEST_CASE("TPSTransform forward and inverse reproduce their GCPs", "[projection][tps][tps_transform]")
{
    auto gcps = affineGrid();
    // Make it non-affine so the radial terms actually matter
    for (auto &g : gcps)
        g.lat += 0.5 * std::sin(g.x * 0.05) * std::cos(g.y * 0.03);

    TPSTransform tps;
    REQUIRE(tps.init(gcps) == 0);
    for (auto &g : gcps)
    {
        double x, y, lon, lat;
        tps.forward(g.lon, g.lat, x, y);
        CHECK_THAT(x, WithinAbs(g.x, 1e-6));
        CHECK_THAT(y, WithinAbs(g.y, 1e-6));
        tps.inverse(g.x, g.y, lon, lat);
        CHECK_THAT(lon, WithinAbs(g.lon, 1e-8));
        CHECK_THAT(lat, WithinAbs(g.lat, 1e-8));
    }
}

TEST_CASE("TPSTransform interpolates an affine mapping exactly between GCPs", "[projection][tps][tps_transform]")
{
    TPSTransform tps(affineGrid());
    for (double y = 3; y < 100; y += 17)
    {
        for (double x = 1; x < 100; x += 13)
        {
            GCP e = affineGcp(x, y);
            double lon, lat, fx, fy;
            tps.inverse(x, y, lon, lat);
            CHECK_THAT(lon, WithinAbs(e.lon, 1e-8));
            CHECK_THAT(lat, WithinAbs(e.lat, 1e-8));
            tps.forward(e.lon, e.lat, fx, fy);
            CHECK_THAT(fx, WithinAbs(x, 1e-6));
            CHECK_THAT(fy, WithinAbs(y, 1e-6));
        }
    }
}

TEST_CASE("TPSTransform ignores duplicated GCPs", "[projection][tps][tps_transform]")
{
    auto gcps = affineGrid();
    const size_t n = gcps.size();
    gcps.push_back(gcps[3]); // Exact duplicate
    GCP same_px = gcps[5];
    same_px.lon += 1; // Same pixel, different position: dropped with a warning
    gcps.push_back(same_px);
    GCP same_pos = gcps[7];
    same_pos.x += 0.5; // Same position, different pixel: dropped too
    gcps.push_back(same_pos);

    TPSTransform tps;
    REQUIRE(tps.init(gcps) == 0);
    CHECK(tps.getRawForward()._nof_points == (int)n);

    double lon, lat;
    tps.inverse(gcps[5].x, gcps[5].y, lon, lat);
    CHECK_THAT(lon, WithinAbs(gcps[5].lon, 1e-8));
}

TEST_CASE("TPSTransform can be re-initialised with new GCPs", "[projection][tps][tps_transform]")
{
    TPSTransform tps(affineGrid());
    std::vector<GCP> shifted = affineGrid();
    for (auto &g : shifted)
        g.lon += 100;
    REQUIRE(tps.init(shifted) == 0);

    double lon, lat;
    tps.inverse(40, 50, lon, lat);
    CHECK_THAT(lon, WithinAbs(affineGcp(40, 50).lon + 100, 1e-8));
    CHECK_THAT(lat, WithinAbs(affineGcp(40, 50).lat, 1e-8));
}

TEST_CASE("TPSTransform init fails when there are no GCPs to solve", "[projection][tps][tps_transform]")
{
    TPSTransform tps;
    CHECK(tps.init({}) == 1);
}
