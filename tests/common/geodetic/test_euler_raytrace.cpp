#include "common/geodetic/euler_raytrace.h"
#include "common/geodetic/vincentys_calculations.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

extern "C"
{
#include "libs/predict/unsorted.h"
}

using Catch::Matchers::WithinAbs;

namespace
{
    // Arbitrary fixed predict time (days since 1979-12-31 00:00 UTC), so the tests are deterministic
    const double PREDICT_TIME = 16000.3;
    const double JULIAN_OFFSET = 2444238.5;

    // ECI position (km) of a satellite above (lat, lon) at alt km, at PREDICT_TIME
    void satEci(double lat_deg, double lon_deg, double alt_km, double pos[3])
    {
        geodetic_t g;
        g.lat = lat_deg * DEG_TO_RAD;
        g.lon = lon_deg * DEG_TO_RAD;
        g.alt = alt_km;
        double vel[3];
        Calculate_User_PosVel(PREDICT_TIME + JULIAN_OFFSET, &g, pos, vel);
    }

    int trace(double lat, double lon, double alt, const double vel[3], geodetic::euler_coords_t pointing, geodetic::geodetic_coords_t &out)
    {
        double pos[3];
        satEci(lat, lon, alt, pos);
        return geodetic::raytrace_to_earth(PREDICT_TIME, pos, vel, pointing, out);
    }

    const double NORTH_VEL[3] = {0, 0, 7.5}; // Due north when over the equator

    double lonDiff(double a, double b)
    {
        double d = std::fmod(std::fabs(a - b), 360.0);
        return d > 180 ? 360 - d : d;
    }
} // namespace

TEST_CASE("raytrace_to_earth nadir pointing hits the sub-satellite point", "[common][geodetic][raytrace]")
{
    for (double lat : {-70.0, -20.0, 0.0, 35.0, 60.0})
    {
        for (double lon : {-150.0, 0.0, 45.0, 179.0})
        {
            INFO("lat " << lat << " lon " << lon);
            // Any velocity perpendicular-ish to nadir is fine for pure nadir pointing
            double vel[3] = {-std::sin(lon * DEG_TO_RAD) * 7.5, std::cos(lon * DEG_TO_RAD) * 7.5, 1.0};
            geodetic::geodetic_coords_t out;
            REQUIRE(trace(lat, lon, 800, vel, {0, 0, 0}, out) == 0);
            CHECK_FALSE(out.is_radians);
            CHECK_THAT(out.lat, WithinAbs(lat, 1e-6));
            CHECK(lonDiff(out.lon, lon) < 1e-6);
            CHECK_THAT(out.alt, WithinAbs(0, 1e-3));
        }
    }
}

TEST_CASE("raytrace_to_earth returns longitudes in (-180, 180]", "[common][geodetic][raytrace]")
{
    geodetic::geodetic_coords_t out;
    REQUIRE(trace(10, -160, 800, NORTH_VEL, {0, 0, 0}, out) == 0);
    CHECK_THAT(out.lon, WithinAbs(-160, 1e-6));
    REQUIRE(trace(10, 200, 800, NORTH_VEL, {0, 0, 0}, out) == 0);
    CHECK_THAT(out.lon, WithinAbs(-160, 1e-6));
}

TEST_CASE("raytrace_to_earth roll matches the spherical geometry in the equatorial plane", "[common][geodetic][raytrace]")
{
    // The equatorial section of the ellipsoid is a circle of radius a, so for a satellite over the
    // equator flying north, rolling sweeps the ray along the equator and the ground offset is
    // exactly gamma = asin((a + h) / a * sin(theta)) - theta.
    const double a = 6378.137, h = 800;
    for (double roll : {5.0, 20.0, 45.0})
    {
        INFO("roll " << roll);
        double theta = roll * DEG_TO_RAD;
        double gamma = (std::asin((a + h) / a * std::sin(theta)) - theta) * RAD_TO_DEG;

        geodetic::geodetic_coords_t plus, minus;
        REQUIRE(trace(0, 30, h, NORTH_VEL, {roll, 0, 0}, plus) == 0);
        REQUIRE(trace(0, 30, h, NORTH_VEL, {-roll, 0, 0}, minus) == 0);

        CHECK_THAT(plus.lat, WithinAbs(0, 1e-6));
        CHECK_THAT(minus.lat, WithinAbs(0, 1e-6));
        CHECK_THAT(lonDiff(plus.lon, 30), WithinAbs(gamma, 1e-6));
        CHECK_THAT(lonDiff(minus.lon, 30), WithinAbs(gamma, 1e-6));
        // Opposite rolls land on opposite sides of the ground track
        CHECK((plus.lon - 30) * (minus.lon - 30) < 0);
    }
}

TEST_CASE("raytrace_to_earth pitch moves along track symmetrically", "[common][geodetic][raytrace]")
{
    geodetic::geodetic_coords_t fwd, back;
    REQUIRE(trace(0, 30, 800, NORTH_VEL, {0, 10, 0}, fwd) == 0);
    REQUIRE(trace(0, 30, 800, NORTH_VEL, {0, -10, 0}, back) == 0);
    // Pitch stays in the meridian plane
    CHECK(lonDiff(fwd.lon, 30) < 1e-6);
    CHECK(lonDiff(back.lon, 30) < 1e-6);
    // The ellipsoid is symmetric about the equator
    CHECK(std::fabs(fwd.lat) > 0.5);
    CHECK_THAT(fwd.lat, WithinAbs(-back.lat, 1e-6));
}

TEST_CASE("raytrace_to_earth yaw alone does not move the nadir point", "[common][geodetic][raytrace]")
{
    geodetic::geodetic_coords_t a, b;
    REQUIRE(trace(25, 60, 800, NORTH_VEL, {0, 0, 0}, a) == 0);
    REQUIRE(trace(25, 60, 800, NORTH_VEL, {0, 0, 73}, b) == 0);
    CHECK_THAT(a.lat, WithinAbs(b.lat, 1e-9));
    CHECK(lonDiff(a.lon, b.lon) < 1e-9);
}

TEST_CASE("raytrace_to_earth yaw of 90 degrees turns roll into pitch", "[common][geodetic][raytrace]")
{
    geodetic::geodetic_coords_t yawed, pitched_p, pitched_m;
    REQUIRE(trace(0, 30, 800, NORTH_VEL, {10, 0, 90}, yawed) == 0);
    REQUIRE(trace(0, 30, 800, NORTH_VEL, {0, 10, 0}, pitched_p) == 0);
    REQUIRE(trace(0, 30, 800, NORTH_VEL, {0, -10, 0}, pitched_m) == 0);
    CHECK(lonDiff(yawed.lon, 30) < 1e-6);
    CHECK_THAT(std::fabs(yawed.lat), WithinAbs(std::fabs(pitched_p.lat), 1e-6));
    bool matches = std::fabs(yawed.lat - pitched_p.lat) < 1e-6 || std::fabs(yawed.lat - pitched_m.lat) < 1e-6;
    CHECK(matches);
}

TEST_CASE("raytrace_to_earth reports a miss when pointing beyond the limb or away from Earth", "[common][geodetic][raytrace]")
{
    // At 800 km the limb is at asin(a / (a + h)) ~ 62.7 degrees off nadir
    geodetic::geodetic_coords_t out;
    CHECK(trace(0, 30, 800, NORTH_VEL, {62, 0, 0}, out) == 0);
    CHECK(trace(0, 30, 800, NORTH_VEL, {64, 0, 0}, out) == 1);
    CHECK(trace(0, 30, 800, NORTH_VEL, {0, 80, 0}, out) == 1);
    CHECK(trace(0, 30, 800, NORTH_VEL, {0, 180, 0}, out) == 1);
}

TEST_CASE("raytrace_to_earth_old nadir pointing on the equator hits the sub-satellite point", "[common][geodetic][raytrace]")
{
    // The old implementation points at the Earth's centre rather than along the ellipsoid normal,
    // so it only coincides with the geodetic sub-satellite point on the equator.
    for (double lon : {-120.0, 0.0, 30.0, 179.0})
    {
        INFO("lon " << lon);
        geodetic::geodetic_coords_t out;
        REQUIRE(geodetic::raytrace_to_earth_old({0, lon, 800}, {0, 0, 0}, out) == 0);
        // Output is in radians
        CHECK(out.is_radians);
        out.toDegs();
        CHECK_THAT(out.lat, WithinAbs(0, 1e-9));
        CHECK(lonDiff(out.lon, lon) < 1e-9);
        CHECK_THAT(out.alt, WithinAbs(0, 1e-6));
    }
}

TEST_CASE("raytrace_to_earth_old off-nadir ground distance matches spherical geometry on the equator", "[common][geodetic][raytrace]")
{
    const double a = 6378.137, h = 800;
    // At (0, 30) the local "up" is +z of the look frame, so roll swings the ray along the meridian
    // and pitch along the equator. Only the equatorial direction is exact on the ellipsoid.
    for (double angle : {10.0, 30.0})
    {
        INFO("angle " << angle);
        double theta = angle * DEG_TO_RAD;
        double gamma = (std::asin((a + h) / a * std::sin(theta)) - theta) * RAD_TO_DEG;
        geodetic::geodetic_coords_t p, m;
        REQUIRE(geodetic::raytrace_to_earth_old({0, 30, h}, {0, angle, 0}, p) == 0);
        REQUIRE(geodetic::raytrace_to_earth_old({0, 30, h}, {0, -angle, 0}, m) == 0);
        p.toDegs();
        m.toDegs();
        CHECK_THAT(p.lat, WithinAbs(0, 1e-9));
        CHECK_THAT(m.lat, WithinAbs(0, 1e-9));
        CHECK_THAT(lonDiff(p.lon, 30), WithinAbs(gamma, 1e-6));
        CHECK_THAT(lonDiff(m.lon, 30), WithinAbs(gamma, 1e-6));
        CHECK((p.lon - 30) * (m.lon - 30) < 0);
    }
}

TEST_CASE("raytrace_to_earth_old reports a miss beyond the limb", "[common][geodetic][raytrace]")
{
    geodetic::geodetic_coords_t out;
    CHECK(geodetic::raytrace_to_earth_old({0, 30, 800}, {70, 0, 0}, out) == 1);
    CHECK(geodetic::raytrace_to_earth_old({0, 30, 800}, {0, 180, 0}, out) == 1);
}
