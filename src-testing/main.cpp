/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "init.h"
#include "logger.h"
#include "common/tracking/tle.h"
#include "libs/predict/predict.h"
#include "common/geodetic/geodetic_coordinates.h"
#include "common/geodetic/euler_raytrace.h"
#include <unistd.h>

int main(int argc, char *argv[])
{
    initLogger();

    // We don't wanna spam with init this time around
    logger->set_level(slog::LOG_OFF);
    satdump::initSatdump();
    logger->set_level(slog::LOG_TRACE);

    predict_orbital_elements_t *satellite_object;
    predict_position satellite_orbit;

    auto tle = satdump::general_tle_registry.get_from_norad(40069).value();

    satellite_object = predict_parse_tle(tle.line1.c_str(), tle.line2.c_str());

    while (1)
    {
        time_t utc_time = time(0);

        auto utc_jul = predict_to_julian_double(utc_time);
        predict_orbit(satellite_object, &satellite_orbit, utc_jul);

        auto position = geodetic::geodetic_coords_t(satellite_orbit.latitude, satellite_orbit.longitude, satellite_orbit.altitude, true).toDegs();

        geodetic::geodetic_coords_t earth_point, earth_point2;

        geodetic::raytrace_to_earth(position, {50, 0, 0}, earth_point);
        earth_point.toDegs();

        geodetic::raytrace_to_earth_new(utc_jul, satellite_orbit.position, satellite_orbit.velocity, {50, 0, 0}, earth_point2);

        logger->info("%f %f ---- %f %f ---- %f %f",
                     position.lon, position.lat,
                     earth_point.lon, earth_point.lat,
                     earth_point2.lon, earth_point2.lat);

        sleep(1);
    }

    predict_destroy_orbital_elements(satellite_object);
}