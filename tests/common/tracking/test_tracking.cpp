#include "common/tracking/tracking.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

namespace
{
    // ISS (ZARYA), 2008-09-20 (see test_tle.cpp); epoch 08264.51782528 = 2008-09-20 12:25:40.104 UTC
    const satdump::TLE ISS_TLE = {25544, "ISS (ZARYA)",
                                  "1 25544U 98067A   08264.51782528 -.00002182  00000-0 -11606-4 0  2927",
                                  "2 25544  51.6416 247.4627 0006703 130.5360 325.0288 15.72125391563537", 0};
    const double ISS_EPOCH_UNIX = 1221913540.104;

    // Mean motion 15.72125391 rev/day -> period 86400 / 15.72125391 s
    const double ISS_PERIOD = 86400.0 / 15.72125391;

    double norm3(const double v[3]) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }
} // namespace

TEST_CASE("predict_parse_tle decodes every field of the ISS reference TLE", "[common][tracking][tracking]")
{
    // SatelliteTracker hands the TLE lines straight to libpredict; check its parse field by field
    predict_orbital_elements_t *e = predict_parse_tle(ISS_TLE.line1.c_str(), ISS_TLE.line2.c_str());
    REQUIRE(e != nullptr);
    CHECK(e->satellite_number == 25544);
    CHECK(e->element_number == 292);
    CHECK(std::string(e->designator) == "98067A");
    CHECK(e->epoch_year == 8);
    CHECK_THAT(e->epoch_day, WithinAbs(264.51782528, 1e-9));
    CHECK_THAT(e->derivative_mean_motion, WithinAbs(-0.00002182, 1e-12));
    CHECK_THAT(e->second_derivative_mean_motion, WithinAbs(0, 1e-15));
    CHECK_THAT(e->bstar_drag_term, WithinAbs(-0.11606e-4, 1e-12));
    CHECK_THAT(e->inclination, WithinAbs(51.6416, 1e-9));
    CHECK_THAT(e->right_ascension, WithinAbs(247.4627, 1e-9));
    CHECK_THAT(e->eccentricity, WithinAbs(0.0006703, 1e-12));
    CHECK_THAT(e->argument_of_perigee, WithinAbs(130.5360, 1e-9));
    CHECK_THAT(e->mean_anomaly, WithinAbs(325.0288, 1e-9));
    CHECK_THAT(e->mean_motion, WithinAbs(15.72125391, 1e-9));
    CHECK(e->revolutions_at_epoch == 56353);
    CHECK_FALSE(predict_is_geosynchronous(e));
    predict_destroy_orbital_elements(e);
}

TEST_CASE("SatelliteTracker ISS altitude and speed are consistent with its orbit", "[common][tracking][tracking]")
{
    // n = 15.72125391 rev/day gives a = (mu / n^2)^(1/3) ~ 6732.6 km and e = 0.00067, so the radius is
    // 6728-6737 km and the height over the ellipsoid (local radius 6365-6378 km at |lat| < 52)
    // is ~350-372 km; vis-viva gives ~7.69 km/s. SGP4's osculating radius additionally swings by
    // ~10 km around the mean orbit (J2 short-period terms), so the radius bounds are wider.
    satdump::SatelliteTracker tracker(ISS_TLE);
    for (double t = 0; t < ISS_PERIOD; t += 120)
    {
        INFO("t = epoch + " << t);
        geodetic::geodetic_coords_t pos = tracker.get_sat_position_at(ISS_EPOCH_UNIX + t);
        CHECK_FALSE(pos.is_radians);
        CHECK(pos.alt > 335);
        CHECK(pos.alt < 385);
        CHECK(std::fabs(pos.lat) <= 52.0);
        // Longitude comes from libpredict in [0, 360)
        CHECK(pos.lon >= 0);
        CHECK(pos.lon < 360);

        predict_position raw = tracker.get_sat_position_at_raw(ISS_EPOCH_UNIX + t);
        double r = norm3(raw.position);
        CHECK(r > 6710);
        CHECK(r < 6750);
        CHECK(norm3(raw.velocity) > 7.6);
        CHECK(norm3(raw.velocity) < 7.8);
    }
}

TEST_CASE("SatelliteTracker ISS ground track reaches the inclination latitude", "[common][tracking][tracking]")
{
    // The geocentric latitude peaks at the 51.64 deg inclination; geodetic latitude is up to
    // ~0.19 deg higher there.
    satdump::SatelliteTracker tracker(ISS_TLE);
    double max_lat = -90, min_lat = 90;
    for (double t = 0; t < ISS_PERIOD; t += 10)
    {
        geodetic::geodetic_coords_t pos = tracker.get_sat_position_at(ISS_EPOCH_UNIX + t);
        max_lat = std::max(max_lat, pos.lat);
        min_lat = std::min(min_lat, pos.lat);
    }
    CHECK(max_lat > 51.5);
    CHECK(max_lat < 52.0);
    CHECK(min_lat < -51.5);
    CHECK(min_lat > -52.0);
}

TEST_CASE("SatelliteTracker ISS returns to the same orbital radius one period later", "[common][tracking][tracking]")
{
    satdump::SatelliteTracker tracker(ISS_TLE);
    predict_position a = tracker.get_sat_position_at_raw(ISS_EPOCH_UNIX);
    double r0 = norm3(a.position);
    predict_position b = tracker.get_sat_position_at_raw(ISS_EPOCH_UNIX + ISS_PERIOD);
    // Drag and J2 shift things a little over one orbit; a few km is still tight compared to the
    // ~9 km perigee/apogee spread and ~13000 km diameter
    CHECK_THAT(norm3(b.position), WithinAbs(r0, 2.0));
    double dot = (a.position[0] * b.position[0] + a.position[1] * b.position[1] + a.position[2] * b.position[2]) / (r0 * norm3(b.position));
    CHECK(dot > std::cos(1.0 * M_PI / 180)); // Back within 1 degree in inertial space
}

TEST_CASE("SatelliteTracker observed from the sub-satellite point is at zenith", "[common][tracking][tracking]")
{
    satdump::SatelliteTracker tracker(ISS_TLE);
    for (double t : {0.0, 1000.0, 2500.0})
    {
        INFO("t = epoch + " << t);
        geodetic::geodetic_coords_t sub = tracker.get_sat_position_at(ISS_EPOCH_UNIX + t);
        predict_observation obs = tracker.get_observed_position(ISS_EPOCH_UNIX + t, sub.lat, sub.lon, 0);
        CHECK(obs.elevation * RAD_TO_DEG > 89.5);
        CHECK_THAT(obs.range, WithinAbs(sub.alt, 1.0));

        // From the antipode the satellite is far below the horizon
        double anti_lon = sub.lon + 180;
        predict_observation anti = tracker.get_observed_position(ISS_EPOCH_UNIX + t, -sub.lat, anti_lon, 0);
        CHECK(anti.elevation * RAD_TO_DEG < -60);
    }
}

TEST_CASE("SatelliteTracker from ephemeris interpolates position and velocity", "[common][tracking][tracking]")
{
    // Straight-line ephemeris (km, km/s) so linear interpolation is exact between nodes
    nlohmann::json ephem = nlohmann::json::array();
    const double t0 = 1.7e9;
    for (int i = 0; i < 5; i++)
    {
        double t = t0 + i * 10;
        ephem.push_back({{"timestamp", t}, {"x", 7000.0}, {"y", 7.5 * (t - t0)}, {"z", -1.0 * (t - t0)}, {"vx", 0.0}, {"vy", 7.5}, {"vz", -1.0}});
    }
    satdump::SatelliteTracker tracker(ephem);

    for (double dt : {12.5, 20.0, 33.25, 40.0}) // exactly representable offsets from 1.7e9
    {
        INFO("dt " << dt);
        predict_position p = tracker.get_sat_position_at_raw(t0 + dt);
        CHECK_THAT(p.position[0], WithinAbs(7000, 1e-9));
        CHECK_THAT(p.position[1], WithinAbs(7.5 * dt, 1e-9));
        CHECK_THAT(p.position[2], WithinAbs(-dt, 1e-9));
        CHECK_THAT(p.velocity[1], WithinAbs(7.5, 1e-12));
        CHECK_THAT(p.velocity[2], WithinAbs(-1.0, 1e-12));
    }

    // Near the ECI x axis, 7000 km out: latitude close to 0 and altitude close to |r| - a
    geodetic::geodetic_coords_t g = tracker.get_sat_position_at(t0 + 20);
    CHECK_FALSE(g.is_radians);
    CHECK(std::fabs(g.lat) < 2.0);
    double r = std::sqrt(7000.0 * 7000.0 + 150.0 * 150.0 + 20.0 * 20.0);
    CHECK_THAT(g.alt, WithinAbs(r - 6378.137, 1.0));
}
