#include "common/geodetic/vincentys_calculations.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

using Catch::Matchers::WithinAbs;

namespace
{
    double dms(double d, double m, double s) { return d + m / 60.0 + s / 3600.0; }

    double wrap360(double deg)
    {
        deg = std::fmod(deg, 360.0);
        return deg < 0 ? deg + 360.0 : deg;
    }

    double angleDiffDeg(double a, double b)
    {
        double d = wrap360(a - b);
        return d > 180 ? 360 - d : d;
    }

    // Flinders Peak -> Buninyong, the standard test line from Vincenty / Geoscience Australia
    const geodetic::geodetic_coords_t flinders_peak(-dms(37, 57, 3.72030), dms(144, 25, 29.52440), 0);
    const geodetic::geodetic_coords_t buninyong(-dms(37, 39, 10.15610), dms(143, 55, 35.38390), 0);
    const double flinders_distance_km = 54.972271;
    const double flinders_azimuth_deg = dms(306, 52, 5.37);
    const double flinders_reverse_deg = dms(127, 10, 25.07);
} // namespace

TEST_CASE("geodetic_curve_t toRads/toDegs convert azimuths only", "[common][geodetic][vincenty]")
{
    geodetic::geodetic_curve_t c(12.5, 90, 270);
    c.toRads();
    CHECK(c.is_radians);
    CHECK(c.distance == 12.5);
    CHECK_THAT(c.azimuth, WithinAbs(M_PI / 2, 1e-15));
    CHECK_THAT(c.reverse_azimuth, WithinAbs(3 * M_PI / 2, 1e-15));
    c.toRads();
    CHECK_THAT(c.azimuth, WithinAbs(M_PI / 2, 1e-15));
    c.toDegs();
    CHECK_THAT(c.azimuth, WithinAbs(90, 1e-12));
    CHECK_THAT(c.reverse_azimuth, WithinAbs(270, 1e-12));
    CHECK(c.distance == 12.5);
}

TEST_CASE("vincentys_inverse reproduces the Flinders Peak - Buninyong test line", "[common][geodetic][vincenty]")
{
    geodetic::geodetic_curve_t c = geodetic::vincentys_inverse(flinders_peak, buninyong);
    CHECK(c.is_radians);
    // Distance is returned in kilometres; published value is given to the millimetre
    CHECK_THAT(c.distance, WithinAbs(flinders_distance_km, 2e-6));
    c.toDegs();
    // Published azimuths are given to 0.01 arcsecond (~2.8e-6 degrees)
    CHECK_THAT(c.azimuth, WithinAbs(flinders_azimuth_deg, 5e-6));
    CHECK_THAT(c.reverse_azimuth, WithinAbs(flinders_reverse_deg, 5e-6));
}

TEST_CASE("vincentys_inverse accepts radian inputs", "[common][geodetic][vincenty]")
{
    geodetic::geodetic_coords_t a = flinders_peak;
    geodetic::geodetic_coords_t b = buninyong;
    a.toRads();
    b.toRads();
    geodetic::geodetic_curve_t c = geodetic::vincentys_inverse(a, b);
    CHECK_THAT(c.distance, WithinAbs(flinders_distance_km, 2e-6));
}

TEST_CASE("vincentys_inverse is symmetric when swapping endpoints", "[common][geodetic][vincenty]")
{
    geodetic::geodetic_curve_t ab = geodetic::vincentys_inverse(flinders_peak, buninyong).toDegs();
    geodetic::geodetic_curve_t ba = geodetic::vincentys_inverse(buninyong, flinders_peak).toDegs();
    CHECK_THAT(ab.distance, WithinAbs(ba.distance, 1e-9));
    CHECK(angleDiffDeg(ab.azimuth, ba.reverse_azimuth) < 1e-7);
    CHECK(angleDiffDeg(ab.reverse_azimuth, ba.azimuth) < 1e-7);
}

// Bug: vincentys_calculations.cpp:230 (and :84 in vincentys_forward) call the C int abs() on a
// double, so the convergence test truncates to 0 and the iteration always stops after 3 rounds.
// Equatorial lines converge with ratio f per round, leaving ~f^3 relative error (~0.4 m here).
// The unqualified abs() only resolves to the integer ::abs with some standard library headers: it
// does with libstdc++, but most likely not with libc++ or MSVC, where this test passes.
TEST_CASE("vincentys_inverse along the equator is a * delta-lon", "[common][geodetic][vincenty][!mayfail]")
{
    geodetic::geodetic_curve_t c = geodetic::vincentys_inverse({0, 0, 0}, {0, 90, 0}).toDegs();
    CHECK_THAT(c.distance, WithinAbs(6378.137 * M_PI / 2, 1e-6));
    CHECK_THAT(c.azimuth, WithinAbs(90, 1e-9));
    CHECK_THAT(c.reverse_azimuth, WithinAbs(270, 1e-9));

    // Crossing the antimeridian takes the short way round
    geodetic::geodetic_curve_t w = geodetic::vincentys_inverse({0, 179, 0}, {0, -179, 0}).toDegs();
    CHECK_THAT(w.distance, WithinAbs(6378.137 * 2 * DEG_TO_RAD, 1e-6));
    CHECK_THAT(w.azimuth, WithinAbs(90, 1e-9));
}

TEST_CASE("vincentys_inverse along a meridian matches the meridian arc length", "[common][geodetic][vincenty]")
{
    // Meridian arc lengths integrated independently: 0-10 deg and equator-to-pole (quarter meridian)
    geodetic::geodetic_curve_t c = geodetic::vincentys_inverse({0, 20, 0}, {10, 20, 0}).toDegs();
    CHECK_THAT(c.distance, WithinAbs(1105.854833, 1e-5));
    CHECK_THAT(c.azimuth, WithinAbs(0, 1e-9));
    CHECK_THAT(c.reverse_azimuth, WithinAbs(180, 1e-9));

    geodetic::geodetic_curve_t s = geodetic::vincentys_inverse({10, 20, 0}, {0, 20, 0}).toDegs();
    CHECK_THAT(s.distance, WithinAbs(1105.854833, 1e-5));
    CHECK_THAT(s.azimuth, WithinAbs(180, 1e-9));
    CHECK_THAT(s.reverse_azimuth, WithinAbs(0, 1e-9));

    geodetic::geodetic_curve_t q = geodetic::vincentys_inverse({0, 0, 0}, {90, 0, 0});
    CHECK_THAT(q.distance, WithinAbs(10001.965729, 1e-5));
}

TEST_CASE("vincentys_inverse of identical points is zero distance", "[common][geodetic][vincenty]")
{
    geodetic::geodetic_curve_t c = geodetic::vincentys_inverse(flinders_peak, flinders_peak);
    // Azimuths are undefined for a zero-length line, so only the distance is checked
    CHECK_THAT(c.distance, WithinAbs(0, 1e-9));
}

// Bug: Same premature termination as above (int abs() at vincentys_calculations.cpp:230):
// long lines end up several decimetres to half a metre off. As above, this only shows where abs()
// resolves to the integer ::abs (libstdc++, most likely not libc++ or MSVC).
TEST_CASE("vincentys_inverse matches a fully converged reference on long lines", "[common][geodetic][vincenty][!mayfail]")
{
    struct Line
    {
        double lat1, lon1, lat2, lon2, dist_km, az_deg, rev_deg;
    };
    // Reference values from an independent implementation of Vincenty's inverse iterated to 1e-14
    const Line lines[] = {
        {0, 0, 40, 120, 12521.126888275, 45.922102836, 290.528179985},
        {-30, -60, 50, 100, 17228.068584165, 31.122708330, 315.925447551},
        {10, 10, -20, 150, 15603.324181567, 109.363684661, 261.279713590},
        {30, 10, 30.5, 12, 200.305094716, 73.433562894, 254.441197202},
    };
    for (const Line &l : lines)
    {
        INFO(l.lat1 << "," << l.lon1 << " -> " << l.lat2 << "," << l.lon2);
        geodetic::geodetic_curve_t c = geodetic::vincentys_inverse({l.lat1, l.lon1, 0}, {l.lat2, l.lon2, 0}).toDegs();
        CHECK_THAT(c.distance, WithinAbs(l.dist_km, 1e-6));
        CHECK(angleDiffDeg(c.azimuth, l.az_deg) < 1e-8);
        CHECK(angleDiffDeg(c.reverse_azimuth, l.rev_deg) < 1e-8);
    }
}

TEST_CASE("vincentys_inverse azimuths are normalised to [0, 2pi)", "[common][geodetic][vincenty]")
{
    for (double lat2 = -60; lat2 <= 60; lat2 += 20)
    {
        for (double lon2 = -150; lon2 <= 150; lon2 += 50)
        {
            if (lat2 == 0 && lon2 == 0)
                continue;
            geodetic::geodetic_curve_t c = geodetic::vincentys_inverse({0.5, 0.5, 0}, {lat2, lon2, 0});
            INFO("to " << lat2 << ", " << lon2);
            CHECK(c.azimuth >= 0);
            CHECK(c.azimuth < 2 * M_PI);
            CHECK(c.reverse_azimuth >= 0);
            CHECK(c.reverse_azimuth < 2 * M_PI);
            CHECK(c.distance > 0);
            // Nothing on Earth is further than half the equator
            CHECK(c.distance <= 6378.137 * M_PI + 1e-6);
        }
    }
}

// Bug: vincentys_calculations.cpp:38 converts the distance with "s = distance * 1000" (km to m)
// but the WGS84 axes it divides by are in kilometres, so any distance ends up 1000x too long.
// (It also stops iterating after one round because of the int abs() at line 84.)
TEST_CASE("vincentys_forward reproduces the Flinders Peak - Buninyong test line", "[common][geodetic][vincenty][!shouldfail]")
{
    double final_bearing = 0;
    geodetic::geodetic_coords_t end = geodetic::vincentys_forward(flinders_peak, flinders_azimuth_deg * DEG_TO_RAD, flinders_distance_km, final_bearing);
    CHECK(end.is_radians);
    end.toDegs();
    // 1e-8 degrees is ~1 mm
    CHECK_THAT(end.lat, WithinAbs(buninyong.lat, 1e-7));
    CHECK_THAT(end.lon, WithinAbs(buninyong.lon, 1e-7));
    // The final bearing is the forward azimuth at the destination (reverse + 180)
    CHECK(angleDiffDeg(final_bearing * RAD_TO_DEG, flinders_reverse_deg + 180) < 5e-6);
}

TEST_CASE("vincentys_forward with zero distance stays put", "[common][geodetic][vincenty]")
{
    double final_bearing = 0;
    geodetic::geodetic_coords_t end = geodetic::vincentys_forward({12.5, -40.25, 3.0}, 1.0, 0.0, final_bearing);
    end.toDegs();
    CHECK_THAT(end.lat, WithinAbs(12.5, 1e-12));
    CHECK_THAT(end.lon, WithinAbs(-40.25, 1e-12));
    // Altitude is carried through unchanged
    CHECK(end.alt == 3.0);
    CHECK_THAT(final_bearing, WithinAbs(1.0, 1e-12));
}

// Bug: Same unit bug as above (vincentys_calculations.cpp:38): direct then inverse does not
// return the requested distance.
TEST_CASE("vincentys_forward then vincentys_inverse round-trips distance and bearing", "[common][geodetic][vincenty][!shouldfail]")
{
    for (double bearing_deg : {10.0, 95.0, 200.0, 315.0})
    {
        for (double dist_km : {1.0, 100.0, 2500.0})
        {
            INFO("bearing " << bearing_deg << " distance " << dist_km);
            geodetic::geodetic_coords_t start(30.0, 10.0, 0);
            double final_bearing;
            geodetic::geodetic_coords_t end = geodetic::vincentys_forward(start, bearing_deg * DEG_TO_RAD, dist_km, final_bearing);
            geodetic::geodetic_curve_t c = geodetic::vincentys_inverse(start, end).toDegs();
            CHECK_THAT(c.distance, WithinAbs(dist_km, 1e-6));
            CHECK(angleDiffDeg(c.azimuth, bearing_deg) < 1e-7);
        }
    }
}
