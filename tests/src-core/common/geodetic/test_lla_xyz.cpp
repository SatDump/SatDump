#include "common/geodetic/lla_xyz.h"
#include "common/geodetic/wgs84.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

using Catch::Matchers::WithinAbs;

namespace
{
    geodetic::vector toXyz(double lat_deg, double lon_deg, double alt_km)
    {
        geodetic::vector v;
        geodetic::lla2xyz(geodetic::geodetic_coords_t(lat_deg, lon_deg, alt_km).toRads(), v);
        return v;
    }
} // namespace

TEST_CASE("lla2xyz maps WGS84 reference points to known ECEF coordinates", "[common][geodetic][lla_xyz]")
{
    const double a = 6378.137;
    const double b = 6356.752314245;

    // Equator / prime meridian
    geodetic::vector v = toXyz(0, 0, 0);
    CHECK_THAT(v.x, WithinAbs(a, 1e-9));
    CHECK_THAT(v.y, WithinAbs(0, 1e-9));
    CHECK_THAT(v.z, WithinAbs(0, 1e-9));

    // Equator, 90E and 180
    v = toXyz(0, 90, 0);
    CHECK_THAT(v.x, WithinAbs(0, 1e-9));
    CHECK_THAT(v.y, WithinAbs(a, 1e-9));
    v = toXyz(0, 180, 0);
    CHECK_THAT(v.x, WithinAbs(-a, 1e-9));
    CHECK_THAT(v.y, WithinAbs(0, 1e-9));
    v = toXyz(0, -90, 0);
    CHECK_THAT(v.y, WithinAbs(-a, 1e-9));

    // Poles sit on the semi-minor axis
    v = toXyz(90, 0, 0);
    CHECK_THAT(v.x, WithinAbs(0, 1e-9));
    CHECK_THAT(v.z, WithinAbs(b, 1e-6));
    v = toXyz(-90, 123, 0);
    CHECK_THAT(v.z, WithinAbs(-b, 1e-6));

    // 45N 0E, independently computed with N = a / sqrt(1 - e^2 sin^2(lat))
    v = toXyz(45, 0, 0);
    CHECK_THAT(v.x, WithinAbs(4517.590878849, 1e-6));
    CHECK_THAT(v.y, WithinAbs(0, 1e-9));
    CHECK_THAT(v.z, WithinAbs(4487.348408866, 1e-6));

    // Altitude is along the normal: at the equator it simply adds to x
    v = toXyz(0, 0, 100);
    CHECK_THAT(v.x, WithinAbs(a + 100, 1e-9));
}

TEST_CASE("xyz2lla inverts known ECEF reference points", "[common][geodetic][lla_xyz]")
{
    geodetic::geodetic_coords_t lla;

    geodetic::xyz2lla({6378.137, 0, 0}, lla);
    CHECK(lla.is_radians);
    CHECK_THAT(lla.lat, WithinAbs(0, 1e-12));
    CHECK_THAT(lla.lon, WithinAbs(0, 1e-12));
    CHECK_THAT(lla.alt, WithinAbs(0, 1e-9));

    geodetic::xyz2lla({0, -6378.137 - 500, 0}, lla);
    CHECK_THAT(lla.lon, WithinAbs(-M_PI / 2, 1e-12));
    CHECK_THAT(lla.alt, WithinAbs(500, 1e-9));

    // Longitude comes out of atan2, so 180 degrees is reported as +pi
    geodetic::xyz2lla({-6378.137, 0, 0}, lla);
    CHECK_THAT(lla.lon, WithinAbs(M_PI, 1e-12));

    // North pole
    geodetic::xyz2lla({0, 0, 6356.752314245 + 10}, lla);
    CHECK_THAT(lla.lat, WithinAbs(M_PI / 2, 1e-9));
    CHECK_THAT(lla.alt, WithinAbs(10, 1e-6));

    geodetic::xyz2lla({4517.590878849, 0, 4487.348408866}, lla);
    CHECK_THAT(lla.lat, WithinAbs(M_PI / 4, 1e-12));
    CHECK_THAT(lla.alt, WithinAbs(0, 1e-6));
}

TEST_CASE("lla2xyz then xyz2lla round-trips over a global grid", "[common][geodetic][lla_xyz]")
{
    for (double lat = -89.0; lat <= 89.0; lat += 7.0)
    {
        for (double lon = -179.5; lon < 180.0; lon += 15.5)
        {
            for (double alt : {0.0, 0.5, 10.0})
            {
                INFO("lat " << lat << " lon " << lon << " alt " << alt);
                geodetic::vector v = toXyz(lat, lon, alt);
                geodetic::geodetic_coords_t lla;
                geodetic::xyz2lla(v, lla);
                lla.toDegs();
                // Bowring's latitude is essentially exact near the surface. Altitude is
                // |r| - |r_surface| rather than the distance along the normal, which is
                // off by ~alt * 6e-6 (see the satellite-altitude test below).
                CHECK_THAT(lla.lat, WithinAbs(lat, 1e-9));
                CHECK_THAT(lla.lon, WithinAbs(lon, 1e-9));
                CHECK_THAT(lla.alt, WithinAbs(alt, 1e-5 * alt + 1e-9));
            }
        }
    }
}

TEST_CASE("lla2xyz then xyz2lla round-trips at satellite altitudes", "[common][geodetic][lla_xyz]")
{
    // xyz2lla computes altitude as |r| - |r_surface| instead of along the normal,
    // so the error grows like alt * (deflection of vertical)^2 / 2 ~ alt * 6e-6.
    // Allow 1e-5 * alt, i.e. ~8 m at 800 km.
    for (double lat = -85.0; lat <= 85.0; lat += 17.0)
    {
        for (double lon : {-170.0, -45.0, 0.0, 33.3, 120.0})
        {
            for (double alt : {400.0, 800.0, 1500.0})
            {
                INFO("lat " << lat << " lon " << lon << " alt " << alt);
                geodetic::vector v = toXyz(lat, lon, alt);
                geodetic::geodetic_coords_t lla;
                geodetic::xyz2lla(v, lla);
                lla.toDegs();
                CHECK_THAT(lla.lat, WithinAbs(lat, 1e-5));
                CHECK_THAT(lla.lon, WithinAbs(lon, 1e-9));
                CHECK_THAT(lla.alt, WithinAbs(alt, 1e-5 * alt));
            }
        }
    }
}
