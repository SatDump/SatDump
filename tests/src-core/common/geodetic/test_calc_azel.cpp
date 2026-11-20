#include "common/geodetic/calc_azel.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

using Catch::Matchers::WithinAbs;

namespace
{
    // Independent textbook reference: WGS84 geodetic -> ECEF (metres), then ECEF -> ENU at the observer
    void refEcef(double lat_deg, double lon_deg, double alt_km, double out[3])
    {
        const double a = 6378137.0, f = 1.0 / 298.257223563, e2 = f * (2 - f);
        double lat = lat_deg * M_PI / 180, lon = lon_deg * M_PI / 180, h = alt_km * 1000;
        double N = a / std::sqrt(1 - e2 * std::sin(lat) * std::sin(lat));
        out[0] = (N + h) * std::cos(lat) * std::cos(lon);
        out[1] = (N + h) * std::cos(lat) * std::sin(lon);
        out[2] = (N * (1 - e2) + h) * std::sin(lat);
    }

    geodetic::az_el_coords_t refAzEl(geodetic::geodetic_coords_t gnd, geodetic::geodetic_coords_t obj)
    {
        double g[3], o[3];
        refEcef(gnd.lat, gnd.lon, gnd.alt, g);
        refEcef(obj.lat, obj.lon, obj.alt, o);
        double d[3] = {o[0] - g[0], o[1] - g[1], o[2] - g[2]};
        double lat = gnd.lat * M_PI / 180, lon = gnd.lon * M_PI / 180;
        double e = -std::sin(lon) * d[0] + std::cos(lon) * d[1];
        double n = -std::sin(lat) * std::cos(lon) * d[0] - std::sin(lat) * std::sin(lon) * d[1] + std::cos(lat) * d[2];
        double u = std::cos(lat) * std::cos(lon) * d[0] + std::cos(lat) * std::sin(lon) * d[1] + std::sin(lat) * d[2];
        double range = std::sqrt(e * e + n * n + u * u);
        double az = std::atan2(e, n) * 180 / M_PI;
        if (az < 0)
            az += 360;
        return {az, std::asin(u / range) * 180 / M_PI, range};
    }

    double azDiff(double a, double b)
    {
        double d = std::fmod(std::fabs(a - b), 360.0);
        return d > 180 ? 360 - d : d;
    }
} // namespace

TEST_CASE("calc_azel of an object directly overhead is 90 degrees elevation", "[common][geodetic][azel]")
{
    for (double lat : {-60.0, 0.0, 12.3, 45.0, 89.0})
    {
        for (double lon : {-179.0, -30.0, 0.0, 100.0})
        {
            INFO("lat " << lat << " lon " << lon);
            geodetic::az_el_coords_t r = geodetic::calc_azel({lat, lon, 0.1}, {lat, lon, 800.1});
            CHECK_THAT(r.el, WithinAbs(90, 1e-6));
            // Range is reported in metres while altitudes are passed in kilometres
            CHECK_THAT(r.range, WithinAbs(800000, 1e-3));
        }
    }
}

TEST_CASE("calc_azel of an object directly below is -90 degrees elevation", "[common][geodetic][azel]")
{
    geodetic::az_el_coords_t r = geodetic::calc_azel({20, 30, 10}, {20, 30, 0});
    CHECK_THAT(r.el, WithinAbs(-90, 1e-6));
    CHECK_THAT(r.range, WithinAbs(10000, 1e-6));
}

TEST_CASE("calc_azel of the observer itself has a zero range", "[common][geodetic][azel]")
{
    geodetic::az_el_coords_t r = geodetic::calc_azel({20, 30, 0}, {20, 30, 0});
    // Not necessarily exactly 0 in optimized builds (1.4e-10 m seen at -O3 -march=native)
    CHECK_THAT(r.range, WithinAbs(0, 1e-6));
}

// Bug: common/geodetic/calc_azel.cpp:54 only treats the observer itself as zenith when the range is exactly 0
// (`if (range == 0)`). The range comes from subtracting two identical ECR positions and rotating the result,
// so whether it is exactly 0 depends on floating point rounding: unoptimized it is, but at -O3 -march=native
// (where the compiler may e.g. contract to FMA) it is about 1.4e-10, and asin(rot_z / range) of the rounding residue gives an
// arbitrary elevation (-9.74 degrees seen) instead of 90.
TEST_CASE("calc_azel of the observer itself is defined as zenith", "[common][geodetic][azel][!mayfail]")
{
    geodetic::az_el_coords_t r = geodetic::calc_azel({20, 30, 0}, {20, 30, 0});
    CHECK_THAT(r.el, WithinAbs(90, 1e-12));
}

TEST_CASE("calc_azel reports north and east azimuths", "[common][geodetic][azel]")
{
    // Due north on the same meridian
    geodetic::az_el_coords_t n = geodetic::calc_azel({0, 0, 0}, {1, 0, 500});
    CHECK(azDiff(n.az, 0) < 1e-6);
    CHECK(n.el > 0);

    // Due east along the equator (same latitude, so the result is also exact)
    geodetic::az_el_coords_t e = geodetic::calc_azel({0, 0, 0}, {0, 1, 500});
    CHECK(azDiff(e.az, 90) < 1e-6);
    geodetic::az_el_coords_t ref = refAzEl({0, 0, 0}, {0, 1, 500});
    CHECK_THAT(e.el, WithinAbs(ref.el, 1e-9));
    CHECK_THAT(e.range, WithinAbs(ref.range, 1e-6));

    // North-east quadrant at the same latitude
    geodetic::az_el_coords_t ne = geodetic::calc_azel({45, 10, 0}, {45, 12, 800});
    geodetic::az_el_coords_t ne_ref = refAzEl({45, 10, 0}, {45, 12, 800});
    CHECK(azDiff(ne.az, ne_ref.az) < 1e-6);
    CHECK_THAT(ne.el, WithinAbs(ne_ref.el, 1e-6));
    CHECK_THAT(ne.range, WithinAbs(ne_ref.range, 1e-3));
}

TEST_CASE("calc_azel elevation is negative for objects below the horizon", "[common][geodetic][azel]")
{
    // A ground point 20 degrees away is well below the local horizon
    geodetic::az_el_coords_t r = geodetic::calc_azel({0, 0, 0}, {0, 20, 0});
    CHECK(r.el < 0);
    CHECK(r.el > -90);
}

// Bug: calc_azel.cpp:64 uses atan(-e/s) instead of atan2, which only covers the north half of the
// compass (and the s == 0 special case always returns 90). Objects to the south or west get
// azimuths reflected into the wrong quadrant (due south -> 0, due west -> 90, south-east -> 315).
TEST_CASE("calc_azel reports south and west azimuths", "[common][geodetic][azel][!shouldfail]")
{
    CHECK(azDiff(geodetic::calc_azel({0, 0, 0}, {-1, 0, 500}).az, 180) < 1e-6);
    CHECK(azDiff(geodetic::calc_azel({0, 0, 0}, {0, -1, 500}).az, 270) < 1e-6);
    // South-east (~125 degrees); 1 degree tolerance absorbs the separate N bug below
    geodetic::az_el_coords_t se = geodetic::calc_azel({45, 10, 0}, {44, 12, 800});
    geodetic::az_el_coords_t se_ref = refAzEl({45, 10, 0}, {44, 12, 800});
    CHECK(azDiff(se.az, se_ref.az) < 1.0);
}

// Bug: calc_azel.cpp:36-38 converts the *object* to ECEF with the prime-vertical radius N computed
// at the *ground* latitude, so any object at a different latitude is misplaced by up to ~21 km.
TEST_CASE("calc_azel matches a textbook ENU computation for objects at other latitudes", "[common][geodetic][azel][!shouldfail]")
{
    // North-east quadrant only, so the azimuth quadrant issue above does not interfere
    geodetic::geodetic_coords_t gnd(0, 0, 0);
    geodetic::geodetic_coords_t obj(40, 10, 800);
    geodetic::az_el_coords_t r = geodetic::calc_azel(gnd, obj);
    geodetic::az_el_coords_t ref = refAzEl(gnd, obj);
    CHECK(azDiff(r.az, ref.az) < 1e-3);
    CHECK_THAT(r.el, WithinAbs(ref.el, 1e-3));
    CHECK_THAT(r.range, WithinAbs(ref.range, 10.0));
}
