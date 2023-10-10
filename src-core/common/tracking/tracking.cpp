#include "tracking.h"

namespace satdump
{
    SatelliteTracker::SatelliteTracker(TLE tle)
    {
        satellite_object = predict_parse_tle(tle.line1.c_str(), tle.line2.c_str());
    }

    SatelliteTracker::~SatelliteTracker()
    {
        predict_destroy_orbital_elements(satellite_object);
    }

    geodetic::geodetic_coords_t SatelliteTracker::get_sat_position_at(double utc_time)
    {
        predict_orbit(satellite_object, &satellite_orbit, predict_to_julian_double(utc_time));
        return geodetic::geodetic_coords_t(satellite_orbit.latitude, satellite_orbit.longitude, satellite_orbit.altitude, true).toDegs();
    }

    predict_position SatelliteTracker::get_sat_position_at_raw(double utc_time)
    {
        predict_orbit(satellite_object, &satellite_orbit, predict_to_julian_double(utc_time));
        return satellite_orbit;
    }
}