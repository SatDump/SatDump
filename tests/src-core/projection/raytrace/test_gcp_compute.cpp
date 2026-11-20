#include "projection/raytrace/gcp_compute.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <set>

using Catch::Matchers::WithinAbs;
using satdump::projection::compute_gcps;

namespace
{
    // Synthetic raytracer: every line has GCPs at x=0 (lon 10) and x=10 (lon 20) on the equator
    nlohmann::json equatorLinesCfg(int lines)
    {
        nlohmann::json cfg;
        cfg["type"] = "gcps_timestamps_line";
        cfg["width"] = 11;
        cfg["height"] = lines;
        int n = 0;
        for (int y = 0; y < lines; y++)
        {
            cfg["gcps"][n++] = {{"x", 0}, {"y", y}, {"lat", 0.0}, {"lon", 10.0}};
            cfg["gcps"][n++] = {{"x", 10}, {"y", y}, {"lat", 0.0}, {"lon", 20.0}};
        }
        cfg["gcp_cnt"] = n;
        return cfg;
    }
} // namespace

TEST_CASE("compute_gcps requires size and spacing parameters", "[projection][raytrace][gcp_compute]")
{
    nlohmann::json cfg = equatorLinesCfg(3);
    cfg["gcp_spacing_x"] = 5;
    CHECK_THROWS(compute_gcps(cfg)); // No gcp_spacing_y

    cfg = equatorLinesCfg(3);
    cfg["gcp_spacing_y"] = 1;
    CHECK_THROWS(compute_gcps(cfg)); // No gcp_spacing_x
}

TEST_CASE("compute_gcps samples a raytracer on the spacing grid plus the last row and column", "[projection][raytrace][gcp_compute]")
{
    nlohmann::json cfg = equatorLinesCfg(4);
    cfg["gcp_spacing_x"] = 4;
    cfg["gcp_spacing_y"] = 2;
    auto gcps = compute_gcps(cfg);

    std::set<double> xs, ys;
    for (auto &g : gcps)
    {
        xs.insert(g.x);
        ys.insert(g.y);
        // Each GCP is the raytraced position of its pixel: lon = 10 + x on the equator
        CHECK_THAT(g.lat, WithinAbs(0, 1e-9));
        CHECK_THAT(g.lon, WithinAbs(10 + g.x, 1e-9));
    }
    CHECK(xs == std::set<double>{0, 4, 8, 10});
    CHECK(ys == std::set<double>{0, 2, 3});
}

TEST_CASE("compute_gcps works on standard projections", "[projection][raytrace][gcp_compute]")
{
    nlohmann::json cfg = {{"type", "equirec"}, {"offset_x", -180.0}, {"offset_y", 90.0},    {"scalar_x", 1.0},    {"scalar_y", -1.0},
                          {"width", 360},      {"height", 180},      {"gcp_spacing_x", 90}, {"gcp_spacing_y", 60}};
    auto gcps = compute_gcps(cfg);
    // x in {0, 90, 180, 270, 359}, y in {0, 60, 120, 179}
    CHECK(gcps.size() == 5 * 4);
    for (auto &g : gcps)
    {
        CHECK_THAT(g.lon, WithinAbs(g.x - 180, 1e-9));
        CHECK_THAT(g.lat, WithinAbs(90 - g.y, 1e-9));
    }
}

TEST_CASE("compute_gcps skips pixels that do not hit the Earth", "[projection][raytrace][gcp_compute]")
{
    // 100x100 geostationary image wider than the Earth disk: corners are in space
    nlohmann::json cfg = {{"type", "geos"},    {"altitude", 35786023.0}, {"scalar_x", 120000.0}, {"scalar_y", -120000.0}, {"offset_x", -6.0e6},
                          {"offset_y", 6.0e6}, {"width", 101},           {"height", 101},        {"gcp_spacing_x", 10},   {"gcp_spacing_y", 10}};
    auto gcps = compute_gcps(cfg);
    REQUIRE_FALSE(gcps.empty());
    CHECK(gcps.size() < 12 * 11); // Not every grid point survived

    bool has_center = false;
    for (auto &g : gcps)
    {
        CHECK(g.lat == g.lat); // No NaN
        CHECK(g.lon == g.lon);
        CHECK(std::fabs(g.lat) < 90);
        CHECK(std::hypot(g.x - 50, g.y - 50) < 50); // The disk is ~45 pixels in radius
        if (g.x == 50 && g.y == 50)
        {
            has_center = true;
            CHECK_THAT(g.lat, WithinAbs(0, 1e-9));
            CHECK_THAT(g.lon, WithinAbs(0, 1e-9));
        }
    }
    CHECK(has_center);
}
