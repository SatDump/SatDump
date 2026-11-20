#include "common/geodetic/euler_coordinates.h"
#include "common/geodetic/geodetic_coordinates.h"
#include "common/geodetic/json.h"
#include "common/geodetic/wgs84.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cmath>

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

TEST_CASE("WGS84 constants match the published ellipsoid definition", "[common][geodetic][wgs84]")
{
    // Reference values from NIMA TR8350.2 (kilometres)
    CHECK(geodetic::WGS84::a == 6378.137);
    CHECK_THAT(geodetic::WGS84::f, WithinRel(1.0 / 298.257223563, 1e-15));
    CHECK_THAT(geodetic::WGS84::b, WithinAbs(6356.7523142, 1e-7));
    CHECK_THAT(geodetic::WGS84::es, WithinRel(0.00669437999014, 1e-11));
    CHECK_THAT(geodetic::WGS84::e, WithinRel(0.0818191908426, 1e-11));
    // e2 is the second eccentricity e' (not squared), e'^2 = 0.00673949674228
    CHECK_THAT(geodetic::WGS84::e2 * geodetic::WGS84::e2, WithinRel(0.00673949674228, 1e-11));
    CHECK_THAT(geodetic::WGS84::one_es, WithinRel(1.0 - 0.00669437999014, 1e-13));
    CHECK_THAT(std::sin(geodetic::WGS84::alpha), WithinRel(geodetic::WGS84::e, 1e-14));
    // Third flattening n = (a - b) / (a + b)
    double n = (geodetic::WGS84::a - geodetic::WGS84::b) / (geodetic::WGS84::a + geodetic::WGS84::b);
    CHECK_THAT(geodetic::WGS84::n, WithinRel(n, 1e-10));
}

TEST_CASE("geodetic_coords_t defaults to zero in degrees", "[common][geodetic]")
{
    geodetic::geodetic_coords_t c;
    CHECK(c.lat == 0);
    CHECK(c.lon == 0);
    CHECK(c.alt == 0);
    CHECK_FALSE(c.is_radians);
}

TEST_CASE("geodetic_coords_t toRads/toDegs convert lat/lon but never altitude", "[common][geodetic]")
{
    geodetic::geodetic_coords_t c(45.0, -90.0, 123.5);
    CHECK_FALSE(c.is_radians);

    geodetic::geodetic_coords_t r = c.toRads();
    CHECK(c.is_radians);
    CHECK(r.is_radians);
    CHECK_THAT(c.lat, WithinAbs(M_PI / 4, 1e-15));
    CHECK_THAT(c.lon, WithinAbs(-M_PI / 2, 1e-15));
    CHECK(c.alt == 123.5);
    CHECK(r.lat == c.lat);

    // Converting twice must be a no-op
    c.toRads();
    CHECK_THAT(c.lat, WithinAbs(M_PI / 4, 1e-15));

    c.toDegs();
    CHECK_FALSE(c.is_radians);
    CHECK_THAT(c.lat, WithinAbs(45.0, 1e-12));
    CHECK_THAT(c.lon, WithinAbs(-90.0, 1e-12));
    CHECK(c.alt == 123.5);

    c.toDegs();
    CHECK_THAT(c.lat, WithinAbs(45.0, 1e-12));
}

TEST_CASE("geodetic_coords_t radians constructor keeps values as given", "[common][geodetic]")
{
    geodetic::geodetic_coords_t c(M_PI / 2, M_PI, 1.0, true);
    CHECK(c.is_radians);
    c.toDegs();
    CHECK_THAT(c.lat, WithinAbs(90.0, 1e-12));
    CHECK_THAT(c.lon, WithinAbs(180.0, 1e-12));
}

TEST_CASE("geodetic_coords_t str always prints degrees", "[common][geodetic]")
{
    geodetic::geodetic_coords_t deg(10.5, -20.25, 3.0);
    CHECK(deg.str() == "Lat: 10.500000, Lon: -20.250000, Alt: 3.000000");

    geodetic::geodetic_coords_t rad = geodetic::geodetic_coords_t(10.5, -20.25, 3.0).toRads();
    CHECK(rad.str() == deg.str());
}

TEST_CASE("euler_coords_t toRads/toDegs convert all three angles", "[common][geodetic]")
{
    geodetic::euler_coords_t e(90.0, -45.0, 180.0);
    e.toRads();
    CHECK_THAT(e.roll, WithinAbs(M_PI / 2, 1e-15));
    CHECK_THAT(e.pitch, WithinAbs(-M_PI / 4, 1e-15));
    CHECK_THAT(e.yaw, WithinAbs(M_PI, 1e-15));

    // Idempotent
    e.toRads();
    CHECK_THAT(e.roll, WithinAbs(M_PI / 2, 1e-15));

    e.toDegs();
    CHECK_THAT(e.roll, WithinAbs(90.0, 1e-12));
    CHECK_THAT(e.pitch, WithinAbs(-45.0, 1e-12));
    CHECK_THAT(e.yaw, WithinAbs(180.0, 1e-12));

    geodetic::euler_coords_t d;
    CHECK(d.roll == 0);
    CHECK(d.pitch == 0);
    CHECK(d.yaw == 0);
}

TEST_CASE("euler_coords_t str always prints degrees", "[common][geodetic]")
{
    geodetic::euler_coords_t deg(1.0, 2.0, 3.0);
    CHECK(deg.str() == "Roll: 1.000000, Pitch: 2.000000, Yaw: 3.000000");
    geodetic::euler_coords_t rad(1.0 * DEG_TO_RAD, 2.0 * DEG_TO_RAD, 3.0 * DEG_TO_RAD, true);
    CHECK(rad.str() == deg.str());
}

TEST_CASE("geodetic_coords_t JSON serialisation round-trips", "[common][geodetic][json]")
{
    geodetic::geodetic_coords_t c(51.5, -0.12, 0.035);
    nlohmann::json j = c;
    CHECK(j["lat"].get<double>() == 51.5);
    CHECK(j["lon"].get<double>() == -0.12);
    CHECK(j["alt"].get<double>() == 0.035);
    CHECK(j["is_radians"].get<bool>() == false);

    geodetic::geodetic_coords_t back = j.get<geodetic::geodetic_coords_t>();
    CHECK(back.lat == c.lat);
    CHECK(back.lon == c.lon);
    CHECK(back.alt == c.alt);
    CHECK(back.is_radians == c.is_radians);

    // The radian flag survives as well
    nlohmann::json jr = geodetic::geodetic_coords_t(0.5, 1.0, 2.0, true);
    geodetic::geodetic_coords_t backr = jr.get<geodetic::geodetic_coords_t>();
    CHECK(backr.is_radians);
    CHECK(backr.lat == 0.5);
}

TEST_CASE("geodetic_coords_t JSON deserialisation requires all fields", "[common][geodetic][json]")
{
    nlohmann::json j = nlohmann::json::parse(R"({"lat": 1.0, "lon": 2.0, "alt": 3.0})");
    CHECK_THROWS_AS(j.get<geodetic::geodetic_coords_t>(), nlohmann::json::exception);
}
