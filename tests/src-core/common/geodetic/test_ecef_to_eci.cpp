#include "common/geodetic/ecef_to_eci.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

using Catch::Matchers::WithinAbs;

namespace
{
    // 2000-01-01 12:00:00 UTC (J2000.0). GMST there is 280.46061837 degrees (IAU 1982).
    const double J2000_UNIX = 946728000.0;
    const double GMST_J2000_DEG = 280.46061837;
    const double EARTH_OMEGA = 7.292115e-5; // rad/s

    double norm(double x, double y, double z) { return std::sqrt(x * x + y * y + z * z); }

    double angleDiffDeg(double a, double b)
    {
        double d = std::fmod(std::fabs(a - b), 360.0);
        return d > 180 ? 360 - d : d;
    }
} // namespace

TEST_CASE("ecef_epehem_to_eci rotates a point on the prime meridian by GMST", "[common][geodetic][ecef_to_eci]")
{
    // Input is metres, output kilometres
    double x = 6378137.0, y = 0, z = 0, vx = 0, vy = 0, vz = 0;
    ecef_epehem_to_eci(J2000_UNIX, x, y, z, vx, vy, vz);

    CHECK_THAT(norm(x, y, z), WithinAbs(6378.137, 1e-6));
    CHECK_THAT(z, WithinAbs(0, 1e-9));
    // 1e-4 degrees ~ 11 m at the equator
    CHECK(angleDiffDeg(std::atan2(y, x) * 180 / M_PI, GMST_J2000_DEG) < 1e-4);

    // A point fixed on the Earth only moves with Earth rotation: v = omega x r (km/s)
    CHECK_THAT(vx, WithinAbs(-EARTH_OMEGA * y, 1e-9));
    CHECK_THAT(vy, WithinAbs(EARTH_OMEGA * x, 1e-9));
    CHECK_THAT(vz, WithinAbs(0, 1e-9));
    CHECK_THAT(norm(vx, vy, vz), WithinAbs(EARTH_OMEGA * 6378.137, 1e-9));
}

TEST_CASE("ecef_epehem_to_eci is a rotation about the z axis", "[common][geodetic][ecef_to_eci]")
{
    // JPSS-like orbit radius. The internal ECEF -> LLA -> ECI round trip approximates altitude,
    // which costs a few metres at LEO, so allow 10 m.
    const double r = 7200e3;
    for (double lat_deg : {-60.0, 0.0, 30.0, 85.0})
    {
        for (double lon_deg : {-100.0, 0.0, 45.0, 170.0})
        {
            INFO("lat " << lat_deg << " lon " << lon_deg);
            double lat = lat_deg * M_PI / 180, lon = lon_deg * M_PI / 180;
            double x0 = r * std::cos(lat) * std::cos(lon), y0 = r * std::cos(lat) * std::sin(lon), z0 = r * std::sin(lat);
            double x = x0, y = y0, z = z0, vx = 0, vy = 0, vz = 0;
            ecef_epehem_to_eci(J2000_UNIX + 3600.0, x, y, z, vx, vy, vz);
            CHECK_THAT(norm(x, y, z), WithinAbs(r / 1e3, 0.01));
            CHECK_THAT(z, WithinAbs(z0 / 1e3, 0.01));
            CHECK_THAT(std::hypot(x, y), WithinAbs(std::hypot(x0, y0) / 1e3, 0.01));
        }
    }
}

TEST_CASE("ecef_epehem_to_eci adds Earth rotation to the ECEF velocity", "[common][geodetic][ecef_to_eci]")
{
    // Satellite over the equator moving due north in ECEF at 7 km/s
    const double r = 7178137.0;
    double x = r, y = 0, z = 0, vx = 0, vy = 0, vz = 7000;
    ecef_epehem_to_eci(J2000_UNIX, x, y, z, vx, vy, vz);

    // North stays north, and the eastward component is omega * r
    CHECK_THAT(vz, WithinAbs(7.0, 1e-3));
    double vh = std::hypot(vx, vy);
    CHECK_THAT(vh, WithinAbs(EARTH_OMEGA * r / 1e3, 1e-3));
    // The horizontal velocity is perpendicular to the position and pointing east (counter-clockwise)
    CHECK_THAT((x * vx + y * vy) / (std::hypot(x, y) * vh), WithinAbs(0, 1e-3));
    CHECK(x * vy - y * vx > 0);
}

TEST_CASE("ecef_epehem_to_eci at different times differs only by Earth rotation angle", "[common][geodetic][ecef_to_eci]")
{
    // Half a sidereal day later the same ECEF point is rotated by 180 degrees in ECI
    const double half_sidereal = M_PI / EARTH_OMEGA;
    double x1 = 7000e3, y1 = 0, z1 = 1000e3, vx1 = 0, vy1 = 0, vz1 = 0;
    double x2 = 7000e3, y2 = 0, z2 = 1000e3, vx2 = 0, vy2 = 0, vz2 = 0;
    ecef_epehem_to_eci(J2000_UNIX, x1, y1, z1, vx1, vy1, vz1);
    ecef_epehem_to_eci(J2000_UNIX + half_sidereal, x2, y2, z2, vx2, vy2, vz2);
    double a1 = std::atan2(y1, x1) * 180 / M_PI;
    double a2 = std::atan2(y2, x2) * 180 / M_PI;
    // predict's GMST uses 1.00273790935 rotations per solar day, close to omega; allow 0.01 deg
    CHECK(angleDiffDeg(a2 - a1, 180) < 0.01);
    CHECK_THAT(z1, WithinAbs(z2, 1e-6));
}
