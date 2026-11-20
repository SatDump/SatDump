#include "projection/projection.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using satdump::projection::Projection;

// Note: Projection owns a malloc'd proj::projection_t buffer but has no copy constructor
// or copy assignment, so these tests only ever build Projections in place with from_json().

namespace
{
    // 360x180 full-globe equirectangular image, 1 degree per pixel, (0,0) = 180W 90N
    nlohmann::json globeCfg() { return {{"type", "equirec"}, {"offset_x", -180.0}, {"offset_y", 90.0}, {"scalar_x", 1.0}, {"scalar_y", -1.0}, {"width", 360}, {"height", 180}}; }

    // Two-line synthetic GCP raytracer: line y has GCPs at x=0 (lon 10) and x=10 (lon 20), lat = y
    nlohmann::json gcpLineCfg(int lines)
    {
        nlohmann::json cfg;
        cfg["type"] = "gcps_timestamps_line";
        cfg["width"] = 11;
        cfg["height"] = lines;
        int n = 0;
        for (int y = 0; y < lines; y++)
        {
            cfg["gcps"][n++] = {{"x", 0}, {"y", y}, {"lat", 1.0 * y}, {"lon", 10.0}};
            cfg["gcps"][n++] = {{"x", 10}, {"y", y}, {"lat", 1.0 * y}, {"lon", 20.0}};
            cfg["timestamps"][y] = 1000.0 + y;
        }
        cfg["gcp_cnt"] = n;
        return cfg;
    }
} // namespace

TEST_CASE("Projection from_json requires width and height", "[projection][projection_class]")
{
    nlohmann::json cfg = globeCfg();
    cfg.erase("width");
    Projection p;
    CHECK_THROWS(p.from_json(cfg));

    cfg = globeCfg();
    cfg.erase("height");
    Projection p2;
    CHECK_THROWS(p2.from_json(cfg));
}

TEST_CASE("Projection with a standard config is standard in both directions", "[projection][projection_class]")
{
    Projection p;
    p.from_json(globeCfg());
    CHECK(p.width == 360);
    CHECK(p.height == 180);
    REQUIRE(p.init(true, true));
    CHECK(p.getFwdType() == Projection::PROJ_STANDARD);
    CHECK(p.getInvType() == Projection::PROJ_STANDARD);

    double x = 0, y = 0;
    REQUIRE_FALSE(p.forward(geodetic::geodetic_coords_t(45, -90, 0), x, y));
    CHECK_THAT(x, WithinAbs(90, 1e-9));
    CHECK_THAT(y, WithinAbs(45, 1e-9));

    // Input in radians is converted to degrees before projecting
    REQUIRE_FALSE(p.forward(geodetic::geodetic_coords_t(45 * DEG_TO_RAD, -90 * DEG_TO_RAD, 0, true), x, y));
    CHECK_THAT(x, WithinAbs(90, 1e-9));
    CHECK_THAT(y, WithinAbs(45, 1e-9));

    geodetic::geodetic_coords_t pos;
    REQUIRE_FALSE(p.inverse(270, 120, pos));
    CHECK_THAT(pos.lon, WithinAbs(90, 1e-9));
    CHECK_THAT(pos.lat, WithinAbs(-30, 1e-9));
    CHECK_FALSE(pos.is_radians);
}

TEST_CASE("Projection forward/inverse round-trip on a geostationary config", "[projection][projection_class]")
{
    nlohmann::json cfg = {{"type", "geos"},         {"altitude", 35786023.0}, {"lon0", 0.0},   {"scalar_x", 4000.0}, {"scalar_y", -4000.0},
                          {"offset_x", -5424000.0}, {"offset_y", 5424000.0},  {"width", 2712}, {"height", 2712}};
    Projection p;
    p.from_json(cfg);
    REQUIRE(p.init(true, true));

    for (double lat = -60; lat <= 60; lat += 20)
    {
        for (double lon = -60; lon <= 60; lon += 20)
        {
            double x, y;
            REQUIRE_FALSE(p.forward(geodetic::geodetic_coords_t(lat, lon, 0), x, y));
            geodetic::geodetic_coords_t pos;
            REQUIRE_FALSE(p.inverse(x, y, pos));
            CHECK_THAT(pos.lat, WithinAbs(lat, 1e-8));
            CHECK_THAT(pos.lon, WithinAbs(lon, 1e-8));
        }
    }

    // The far side of the Earth and space are reported as errors
    double x, y;
    CHECK(p.forward(geodetic::geodetic_coords_t(0, 180, 0), x, y));
    geodetic::geodetic_coords_t pos;
    CHECK(p.inverse(0, 0, pos)); // Image corner is off-disk
}

TEST_CASE("Projection applies the channel transform around the projection", "[projection][projection_class]")
{
    // Channel pixels are half the resolution of the reference grid, shifted by 1 reference pixel
    nlohmann::json cfg = globeCfg();
    cfg["width"] = 180;
    cfg["height"] = 90;
    cfg["transform"] = satdump::ChannelTransform().init_affine(2, 2, 1, 1);
    Projection p;
    p.from_json(cfg);
    REQUIRE(p.init(true, true));

    // Channel (10, 20) -> reference (21, 41) -> 159W 49N
    geodetic::geodetic_coords_t pos;
    REQUIRE_FALSE(p.inverse(10, 20, pos));
    CHECK_THAT(pos.lon, WithinAbs(-159, 1e-9));
    CHECK_THAT(pos.lat, WithinAbs(49, 1e-9));

    double x, y;
    REQUIRE_FALSE(p.forward(geodetic::geodetic_coords_t(49, -159, 0), x, y));
    CHECK_THAT(x, WithinAbs(10, 1e-9));
    CHECK_THAT(y, WithinAbs(20, 1e-9));
}

TEST_CASE("Projection applies transform2 before the main transform on inverse", "[projection][projection_class]")
{
    nlohmann::json cfg = globeCfg();
    cfg["transform"] = satdump::ChannelTransform().init_affine(2, 2, 0, 0);
    cfg["transform2"] = satdump::ChannelTransform().init_affine(1, 1, 5, 0);
    Projection p;
    p.from_json(cfg);
    REQUIRE(p.init(true, true));

    // inverse: transform2.forward (x + 5) then transform.forward (* 2)
    geodetic::geodetic_coords_t pos;
    REQUIRE_FALSE(p.inverse(10, 10, pos));
    CHECK_THAT(pos.lon, WithinAbs(-180 + 30, 1e-9));
    CHECK_THAT(pos.lat, WithinAbs(90 - 20, 1e-9));

    double x, y;
    REQUIRE_FALSE(p.forward(pos, x, y));
    CHECK_THAT(x, WithinAbs(10, 1e-9));
    CHECK_THAT(y, WithinAbs(10, 1e-9));
}

TEST_CASE("Projection inverse reports proj_timestamp for standard projections", "[projection][projection_class]")
{
    nlohmann::json cfg = globeCfg();
    cfg["proj_timestamp"] = 1700000000.5;
    Projection p;
    p.from_json(cfg);
    REQUIRE(p.init(false, true));

    geodetic::geodetic_coords_t pos;
    double t = -1;
    REQUIRE_FALSE(p.inverse(5, 5, pos, &t));
    CHECK(t == 1700000000.5);

    // Without the key the timestamp is reported as -1
    Projection p2;
    p2.from_json(globeCfg());
    REQUIRE(p2.init(false, true));
    t = 0;
    REQUIRE_FALSE(p2.inverse(5, 5, pos, &t));
    CHECK(t == -1);
}

TEST_CASE("Projection to_json returns the config with width and height", "[projection][projection_class]")
{
    nlohmann::json cfg = globeCfg();
    cfg["custom_key"] = "kept";
    Projection p;
    p.from_json(cfg);
    nlohmann::json out;
    p.to_json(out);
    CHECK(out == cfg);

    p.width = 720;
    p.height = 360;
    p.to_json(out);
    CHECK(out["width"] == 720);
    CHECK(out["height"] == 360);
    CHECK(out["custom_key"] == "kept");
}

TEST_CASE("Projection with an unknown type is invalid", "[projection][projection_class]")
{
    nlohmann::json cfg = {{"type", "definitely_not_a_projection"}, {"width", 10}, {"height", 10}};
    Projection p;
    p.from_json(cfg);
    CHECK_FALSE(p.init(true, true));
    CHECK(p.getFwdType() == Projection::PROJ_INVALID);
    CHECK(p.getInvType() == Projection::PROJ_INVALID);

    double x, y;
    geodetic::geodetic_coords_t pos;
    CHECK_THROWS(p.forward(geodetic::geodetic_coords_t(0, 0, 0), x, y));
    CHECK_THROWS(p.inverse(0, 0, pos));
    CHECK(p.forward(geodetic::geodetic_coords_t(0, 0, 0), x, y, false));
    CHECK(p.inverse(0, 0, pos, nullptr, false));
}

TEST_CASE("Projection with a raytracer config is raytraced inverse-only by default", "[projection][projection_class][raytrace]")
{
    Projection p;
    p.from_json(gcpLineCfg(3));
    REQUIRE(p.init(false, true));
    CHECK(p.getFwdType() == Projection::PROJ_INVALID);
    CHECK(p.getInvType() == Projection::PROJ_RAYTRACER);

    geodetic::geodetic_coords_t pos;
    REQUIRE_FALSE(p.inverse(5, 0, pos));
    CHECK_THAT(pos.lon, WithinAbs(15, 1e-9));
    CHECK_THAT(pos.lat, WithinAbs(0, 1e-9));

    // Off the equator the interpolation happens in a rotated frame, so it is only
    // approximately linear in lat/lon
    double t = -1;
    REQUIRE_FALSE(p.inverse(5, 2, pos, &t));
    CHECK_THAT(pos.lon, WithinAbs(15, 0.05));
    CHECK_THAT(pos.lat, WithinAbs(2, 0.05));
    CHECK(t == 1002);

    double x, y;
    CHECK_THROWS(p.forward(pos, x, y));
}

TEST_CASE("Projection forward on a raytracer config uses a TPS through the GCPs", "[projection][projection_class][raytrace]")
{
    nlohmann::json cfg = gcpLineCfg(5);
    cfg["gcp_spacing_x"] = 5;
    cfg["gcp_spacing_y"] = 1;
    Projection p;
    p.from_json(cfg);
    REQUIRE(p.init(true, true));
    CHECK(p.getFwdType() == Projection::PROJ_THINPLATESPLINE);
    CHECK(p.getInvType() == Projection::PROJ_RAYTRACER);

    // The TPS is exact at its GCPs (every pixel at x = 0, 5, 10 of every line)
    for (int y = 0; y < 5; y++)
    {
        for (int x : {0, 5, 10})
        {
            geodetic::geodetic_coords_t pos;
            REQUIRE_FALSE(p.inverse(x, y, pos));
            double fx, fy;
            REQUIRE_FALSE(p.forward(pos, fx, fy));
            CHECK_THAT(fx, WithinAbs(x, 1e-6));
            CHECK_THAT(fy, WithinAbs(y, 1e-6));
        }
    }
}
