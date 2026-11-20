#include "common/geodetic/vincentys_calculations.h"
#include "common/tracking/tracking.h"
#include "projection/proj_test_helpers.h"
#include "projection/raytrace/satellite_raytracer.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using satdump::projection::get_satellite_raytracer;

namespace
{
    using proj_test::lonDiff;

    // ISS (ZARYA) 2008-09-20 element set, as used by tests/src-core/common/tracking
    const satdump::TLE ISS_TLE = {25544, "ISS (ZARYA)", "1 25544U 98067A   08264.51782528 -.00002182  00000-0 -11606-4 0  2927",
                                  "2 25544  51.6416 247.4627 0006703 130.5360 325.0288 15.72125391563537", 0};
    const double EPOCH = 1221913540.0; // Close to the TLE epoch

    nlohmann::json tleJson()
    {
        nlohmann::json j = ISS_TLE;
        return j;
    }

    std::vector<double> lineTimestamps(int n, double step = 1.0)
    {
        std::vector<double> t;
        for (int i = 0; i < n; i++)
            t.push_back(EPOCH + i * step);
        return t;
    }

    nlohmann::json normalLineCfg(const std::string &type = "normal_single_line")
    {
        return {{"type", type}, {"tle", tleJson()}, {"timestamps", lineTimestamps(5)}, {"image_width", 100}, {"scan_angle", 110.0}};
    }

    geodetic::geodetic_coords_t subSatellitePoint(double t)
    {
        satdump::SatelliteTracker tracker(ISS_TLE);
        return tracker.get_sat_position_at(t);
    }

    double distanceKm(const geodetic::geodetic_coords_t &a, const geodetic::geodetic_coords_t &b)
    {
        return geodetic::vincentys_inverse(geodetic::geodetic_coords_t(a.lat, a.lon, 0), geodetic::geodetic_coords_t(b.lat, b.lon, 0)).distance;
    }

    double azimuthDeg(const geodetic::geodetic_coords_t &from, const geodetic::geodetic_coords_t &to)
    {
        return geodetic::vincentys_inverse(geodetic::geodetic_coords_t(from.lat, from.lon, 0), geodetic::geodetic_coords_t(to.lat, to.lon, 0)).azimuth * RAD_TO_DEG;
    }

    nlohmann::json gcpLinesCfg(const std::vector<std::vector<std::array<double, 3>>> &lines)
    {
        nlohmann::json cfg;
        cfg["type"] = "gcps_timestamps_line";
        int n = 0;
        for (size_t y = 0; y < lines.size(); y++)
            for (auto &g : lines[y])
                cfg["gcps"][n++] = {{"x", (int)g[0]}, {"y", (int)y}, {"lat", g[1]}, {"lon", g[2]}};
        cfg["gcp_cnt"] = n;
        return cfg;
    }
} // namespace

TEST_CASE("get_satellite_raytracer rejects unknown types and missing orbit data", "[projection][raytrace]")
{
    CHECK_THROWS(get_satellite_raytracer({{"type", "not_a_raytracer"}}));

    nlohmann::json cfg = normalLineCfg();
    cfg.erase("tle");
    CHECK_THROWS(get_satellite_raytracer(cfg));

    // A TLE with the default (-1) NORAD id is treated as absent
    cfg["tle"] = satdump::TLE();
    cfg["tle"]["name"] = "";
    cfg["tle"]["line1"] = "";
    cfg["tle"]["line2"] = "";
    CHECK_THROWS(get_satellite_raytracer(cfg));
}

TEST_CASE("NormalLineRaytracer nadir pixel hits the sub-satellite point", "[projection][raytrace][normal_line]")
{
    auto rt = get_satellite_raytracer(normalLineCfg());
    REQUIRE(rt);

    for (int y = 0; y < 5; y++)
    {
        geodetic::geodetic_coords_t pos;
        double t = -1;
        // x = image_width / 2 is the zero-roll pixel
        REQUIRE_FALSE(rt->get_position(50, y + 0.5, pos, &t));
        CHECK(t == EPOCH + y);
        auto ssp = subSatellitePoint(EPOCH + y);
        CHECK_THAT(pos.lat, WithinAbs(ssp.lat, 1e-3));
        CHECK_THAT(lonDiff(pos.lon, ssp.lon), WithinAbs(0, 1e-3));
        CHECK_FALSE(pos.is_radians);
    }
}

TEST_CASE("NormalLineRaytracer scan edges are symmetric and across track", "[projection][raytrace][normal_line]")
{
    auto rt = get_satellite_raytracer(normalLineCfg());
    geodetic::geodetic_coords_t nadir, left, right, next_nadir;
    REQUIRE_FALSE(rt->get_position(50, 0, nadir));
    REQUIRE_FALSE(rt->get_position(50, 4, next_nadir));
    REQUIRE_FALSE(rt->get_position(0, 0, left));
    REQUIRE_FALSE(rt->get_position(100, 0, right));

    const double dl = distanceKm(nadir, left);
    const double dr = distanceKm(nadir, right);
    CHECK_THAT(dl / dr, WithinAbs(1, 0.01));

    // Spherical-Earth ground distance for a 55 degree off-nadir look from altitude h
    const double R = 6371.0, h = subSatellitePoint(EPOCH).alt, theta = 55 * DEG_TO_RAD;
    const double expected = R * (std::asin((R + h) / R * std::sin(theta)) - theta);
    CHECK_THAT(dl / expected, WithinAbs(1, 0.02));

    // Scan line is perpendicular to the ground track (Earth rotation skews it by a few degrees)
    const double heading = azimuthDeg(nadir, next_nadir);
    CHECK_THAT(std::fabs(lonDiff(azimuthDeg(nadir, left), heading)), WithinAbs(90, 6));
    CHECK_THAT(std::fabs(lonDiff(azimuthDeg(nadir, right), heading)), WithinAbs(90, 6));
    CHECK_THAT(std::fabs(lonDiff(azimuthDeg(nadir, left), azimuthDeg(nadir, right))), WithinAbs(180, 1));
}

TEST_CASE("NormalLineRaytracer invert_scan mirrors the scan", "[projection][raytrace][normal_line]")
{
    auto rt = get_satellite_raytracer(normalLineCfg());
    nlohmann::json cfg = normalLineCfg();
    cfg["invert_scan"] = true;
    auto rti = get_satellite_raytracer(cfg);

    geodetic::geodetic_coords_t a, b;
    REQUIRE_FALSE(rt->get_position(10, 1, a));
    REQUIRE_FALSE(rti->get_position(90, 1, b));
    CHECK_THAT(a.lat, WithinAbs(b.lat, 1e-6));
    CHECK_THAT(lonDiff(a.lon, b.lon), WithinAbs(0, 1e-6));
}

TEST_CASE("NormalLineRaytracer roll_offset shifts the nadir pixel", "[projection][raytrace][normal_line]")
{
    auto rt = get_satellite_raytracer(normalLineCfg());
    nlohmann::json cfg = normalLineCfg();
    cfg["roll_offset"] = 11.0; // 10 pixels worth of the 110 degree / 100 pixel scan
    auto rto = get_satellite_raytracer(cfg);

    geodetic::geodetic_coords_t a, b;
    REQUIRE_FALSE(rt->get_position(60, 2, a));
    REQUIRE_FALSE(rto->get_position(50, 2, b));
    CHECK_THAT(a.lat, WithinAbs(b.lat, 1e-5));
    CHECK_THAT(lonDiff(a.lon, b.lon), WithinAbs(0, 1e-5));
}

TEST_CASE("NormalLineRaytracer rejects lines out of range or without timestamp", "[projection][raytrace][normal_line]")
{
    nlohmann::json cfg = normalLineCfg();
    cfg["timestamps"][2] = -1;
    auto rt = get_satellite_raytracer(cfg);

    geodetic::geodetic_coords_t pos;
    CHECK(rt->get_position(50, -0.5, pos));
    CHECK(rt->get_position(50, 5, pos));
    CHECK(rt->get_position(50, 2.5, pos));
    CHECK_FALSE(rt->get_position(50, 4.9, pos));
}

TEST_CASE("NormalLineRaytracer looking far off-nadir misses the Earth", "[projection][raytrace][normal_line]")
{
    nlohmann::json cfg = normalLineCfg();
    cfg["scan_angle"] = 200.0; // Edges look 100 degrees off nadir, above the horizon
    auto rt = get_satellite_raytracer(cfg);

    geodetic::geodetic_coords_t pos;
    CHECK_FALSE(rt->get_position(50, 0, pos));
    CHECK(rt->get_position(0, 0, pos));
    CHECK(rt->get_position(100, 0, pos));
}

TEST_CASE("NormalLineRaytracerOld nadir pixel hits the sub-satellite point", "[projection][raytrace][normal_line]")
{
    auto rt = get_satellite_raytracer(normalLineCfg("normal_single_line_old"));
    for (int y = 0; y < 5; y++)
    {
        geodetic::geodetic_coords_t pos;
        double t = -1;
        REQUIRE_FALSE(rt->get_position(50, y, pos, &t));
        CHECK(t == EPOCH + y);
        auto ssp = subSatellitePoint(EPOCH + y);
        // The old model points at the geocenter rather than along the ellipsoid normal,
        // which at 51.6N puts its "nadir" ~0.01 degree (~1 km) away from the geodetic one
        CHECK_THAT(pos.lat, WithinAbs(ssp.lat, 0.02));
        CHECK_THAT(lonDiff(pos.lon, ssp.lon), WithinAbs(0, 0.02));
    }

    geodetic::geodetic_coords_t pos;
    CHECK(rt->get_position(50, 5, pos));
    CHECK(rt->get_position(50, -1, pos));
}

TEST_CASE("NormalLineRaytracerOld and NormalLineRaytracer agree across the scan", "[projection][raytrace][normal_line]")
{
    auto rt = get_satellite_raytracer(normalLineCfg());
    auto rto = get_satellite_raytracer(normalLineCfg("normal_single_line_old"));
    for (int x : {0, 25, 75, 100})
    {
        geodetic::geodetic_coords_t a, b;
        REQUIRE_FALSE(rt->get_position(x, 1, a));
        REQUIRE_FALSE(rto->get_position(x, 1, b));
        // Two different attitude models (inertial velocity vs ground track azimuth): a few km apart at most
        CHECK(distanceKm(a, b) < 30);
    }
}

TEST_CASE("NormalPerIFOVRaytracerOld center IFOV pixel hits the sub-satellite point", "[projection][raytrace][normal_per_ifov]")
{
    nlohmann::json cfg = {{"type", "normal_per_ifov_old"},
                          {"tle", tleJson()},
                          {"timestamps", lineTimestamps(4)},
                          {"image_width", 100},
                          {"scan_angle", 100.0},
                          {"ifov_count", 1},
                          {"ifov_x_size", 100},
                          {"ifov_y_size", 1},
                          {"ifov_x_scan_angle", 100.0},
                          {"ifov_y_scan_angle", 1.0}};
    auto rt = get_satellite_raytracer(cfg);

    for (int y = 0; y < 4; y++)
    {
        geodetic::geodetic_coords_t pos;
        double t = -1;
        // Without invert_scan x is mirrored: final_x = 99 - 49 = 50 is the zero-roll IFOV pixel
        REQUIRE_FALSE(rt->get_position(49, y, pos, &t));
        CHECK(t == EPOCH + y);
        auto ssp = subSatellitePoint(EPOCH + y);
        // Old geocentric-pointing model, see NormalLineRaytracerOld above
        CHECK_THAT(pos.lat, WithinAbs(ssp.lat, 0.02));
        CHECK_THAT(lonDiff(pos.lon, ssp.lon), WithinAbs(0, 0.02));
    }

    geodetic::geodetic_coords_t pos;
    CHECK(rt->get_position(49, 4, pos));
}

TEST_CASE("ManualLineRaytracerOld interpolates roll between control points", "[projection][raytrace][manual_line]")
{
    nlohmann::json cfg = {{"type", "manual_single_line_old"}, {"tle", tleJson()}, {"timestamps", lineTimestamps(3)}, {"image_width", 101}};
    cfg["points"]["0"] = {-50.0, 0.0, 0.0};
    cfg["points"]["100"] = {50.0, 0.0, 0.0};
    auto rt = get_satellite_raytracer(cfg);

    geodetic::geodetic_coords_t pos;
    double t = -1;
    REQUIRE_FALSE(rt->get_position(50, 1, pos, &t));
    CHECK(t == EPOCH + 1);
    auto ssp = subSatellitePoint(EPOCH + 1);
    // Old geocentric-pointing model, see NormalLineRaytracerOld above
    CHECK_THAT(pos.lat, WithinAbs(ssp.lat, 0.02));
    CHECK_THAT(lonDiff(pos.lon, ssp.lon), WithinAbs(0, 0.02));

    // Roll is linear in x, so x = 0 and x = 100 land symmetrically around nadir
    geodetic::geodetic_coords_t left, right;
    REQUIRE_FALSE(rt->get_position(0, 1, left));
    REQUIRE_FALSE(rt->get_position(100, 1, right));
    CHECK_THAT(distanceKm(pos, left) / distanceKm(pos, right), WithinAbs(1, 0.01));

    CHECK(rt->get_position(101, 1, pos));
    CHECK(rt->get_position(50, 3, pos));
}

TEST_CASE("TimestampLineGCPsRaytracer reproduces GCPs and interpolates along the equator", "[projection][raytrace][gcps_line]")
{
    auto cfg = gcpLinesCfg({{{0, 0, 10}, {10, 0, 20}, {20, 0, 40}}, {{0, 30, -60}, {20, 30, -40}}});
    auto rt = get_satellite_raytracer(cfg);

    geodetic::geodetic_coords_t pos;
    REQUIRE_FALSE(rt->get_position(0, 0, pos));
    CHECK_THAT(pos.lat, WithinAbs(0, 1e-9));
    CHECK_THAT(pos.lon, WithinAbs(10, 1e-9));
    REQUIRE_FALSE(rt->get_position(10, 0, pos));
    CHECK_THAT(pos.lon, WithinAbs(20, 1e-9));
    REQUIRE_FALSE(rt->get_position(20, 0, pos));
    CHECK_THAT(pos.lon, WithinAbs(40, 1e-9));

    // Linear inside the last segment (exact on the equator)
    REQUIRE_FALSE(rt->get_position(15, 0, pos));
    CHECK_THAT(pos.lat, WithinAbs(0, 1e-9));
    CHECK_THAT(pos.lon, WithinAbs(30, 1e-9));

    // GCPs off the equator are still reproduced exactly
    REQUIRE_FALSE(rt->get_position(0, 1, pos));
    CHECK_THAT(pos.lat, WithinAbs(30, 1e-9));
    CHECK_THAT(pos.lon, WithinAbs(-60, 1e-9));
    REQUIRE_FALSE(rt->get_position(20, 1, pos));
    CHECK_THAT(pos.lat, WithinAbs(30, 1e-9));
    CHECK_THAT(pos.lon, WithinAbs(-40, 1e-9));

    CHECK(rt->get_position(0, 2, pos));
    CHECK(rt->get_position(0, -1, pos));
}

// Bug: timestamp_line_gcps.cpp:133-138 picks the first GCP with x >= the pixel as the
// LEFT end of the interpolation segment. For x in (g[k-1].x, g[k].x) with k < n-1 it
// therefore extrapolates from segment [k, k+1] instead of interpolating in [k-1, k]
// (here x = 5 between lon 10 and 20 returns lon 10 instead of 15).
TEST_CASE("TimestampLineGCPsRaytracer interpolates inside interior GCP segments", "[projection][raytrace][gcps_line][!shouldfail]")
{
    auto cfg = gcpLinesCfg({{{0, 0, 10}, {10, 0, 20}, {20, 0, 40}}});
    auto rt = get_satellite_raytracer(cfg);

    geodetic::geodetic_coords_t pos;
    REQUIRE_FALSE(rt->get_position(5, 0, pos));
    CHECK_THAT(pos.lat, WithinAbs(0, 1e-9));
    CHECK_THAT(pos.lon, WithinAbs(15, 1e-9));
}

TEST_CASE("TimestampLineGCPsRaytracer sorts GCPs by x and handles the antimeridian", "[projection][raytrace][gcps_line]")
{
    auto cfg = gcpLinesCfg({{{10, 0, -175}, {0, 0, 175}}});
    auto rt = get_satellite_raytracer(cfg);

    geodetic::geodetic_coords_t pos;
    REQUIRE_FALSE(rt->get_position(5, 0, pos));
    CHECK_THAT(pos.lat, WithinAbs(0, 1e-9));
    CHECK_THAT(std::fabs(pos.lon), WithinAbs(180, 1e-9));
    REQUIRE_FALSE(rt->get_position(0, 0, pos));
    CHECK_THAT(pos.lon, WithinAbs(175, 1e-9));
    REQUIRE_FALSE(rt->get_position(10, 0, pos));
    CHECK_THAT(pos.lon, WithinAbs(-175, 1e-9));
}

TEST_CASE("TimestampLineGCPsRaytracer requires at least two GCPs on every line", "[projection][raytrace][gcps_line]")
{
    CHECK_THROWS(get_satellite_raytracer(gcpLinesCfg({{{0, 0, 10}}})));
    // A line without any GCP in between also fails
    CHECK_THROWS(get_satellite_raytracer(gcpLinesCfg({{{0, 0, 10}, {10, 0, 20}}, {}, {{0, 1, 10}, {10, 1, 20}}})));
}

TEST_CASE("TimestampLineGCPsRaytracer reports line timestamps", "[projection][raytrace][gcps_line]")
{
    auto cfg = gcpLinesCfg({{{0, 0, 10}, {10, 0, 20}}, {{0, 1, 10}, {10, 1, 20}}, {{0, 2, 10}, {10, 2, 20}}});
    cfg["timestamps"] = {100.0, 101.0, 102.0};
    auto rt = get_satellite_raytracer(cfg);

    geodetic::geodetic_coords_t pos;
    double t = -1;
    REQUIRE_FALSE(rt->get_position(5, 2, pos, &t));
    CHECK(t == 102.0);
    REQUIRE_FALSE(rt->get_position(5, 1.5, pos, &t));
    CHECK(t == 101.0);
}

// Bug: timestamp_line_gcps.cpp:124 only reports the timestamp when `y > 0`, so the
// first image line never gets its timestamp (otime is left untouched).
TEST_CASE("TimestampLineGCPsRaytracer reports the timestamp of the first line", "[projection][raytrace][gcps_line][!shouldfail]")
{
    auto cfg = gcpLinesCfg({{{0, 0, 10}, {10, 0, 20}}, {{0, 1, 10}, {10, 1, 20}}});
    cfg["timestamps"] = {100.0, 101.0};
    auto rt = get_satellite_raytracer(cfg);

    geodetic::geodetic_coords_t pos;
    double t = -1;
    REQUIRE_FALSE(rt->get_position(5, 0, pos, &t));
    CHECK(t == 100.0);
}

TEST_CASE("get_satellite_raytracer interpolates timestamps when requested", "[projection][raytrace][gcps_line]")
{
    // Each timestamp becomes `interpolate_timestamps` lines spaced by the scantime, starting half a scan early
    std::vector<std::vector<std::array<double, 3>>> lines;
    for (int y = 0; y < 6; y++)
        lines.push_back({{0, 0, 10}, {10, 0, 20}});
    auto cfg = gcpLinesCfg(lines);
    cfg["timestamps"] = {100.0, -1.0, 200.0};
    cfg["interpolate_timestamps"] = 2;
    cfg["interpolate_timestamps_scantime"] = 0.5;
    auto rt = get_satellite_raytracer(cfg);

    const double expected[] = {99.5, 100.0, -1, -1, 199.5, 200.0};
    for (int y = 1; y < 6; y++) // Line 0 never reports a timestamp, see the [!shouldfail] test above
    {
        geodetic::geodetic_coords_t pos;
        double t = -42;
        REQUIRE_FALSE(rt->get_position(5, y, pos, &t));
        CHECK(t == expected[y]);
    }
}
