#pragma once

#include "common/geodetic/geodetic_coordinates.h"
#include "libs/predict/predict.h"
#include "tle.h"
#include "interpolator.h"

namespace satdump
{
    class SatelliteTracker
    {
    private:
        predict_orbital_elements_t *satellite_object = nullptr;
        predict_position satellite_orbit;

        LinearInterpolator *interp_x = nullptr;
        LinearInterpolator *interp_y = nullptr;
        LinearInterpolator *interp_z = nullptr;
        LinearInterpolator *interp_vx = nullptr;
        LinearInterpolator *interp_vy = nullptr;
        LinearInterpolator *interp_vz = nullptr;

    public:
        SatelliteTracker(TLE tle);              // From TLE
        SatelliteTracker(nlohmann::json ephem); // From Ephemeris
        ~SatelliteTracker();

        geodetic::geodetic_coords_t get_sat_position_at(double utc_time);
        predict_position get_sat_position_at_raw(double utc_time);

        // TODOREWORK. This is bad but will get re-rewritten so...
        predict_observation get_observed_position(double utc_time, double gs_lat, double gs_lon, double gs_alt)
        {
            predict_observation observation_pos;
            predict_position satellite_orbit2;

            predict_observer_t *observer_station = predict_create_observer("Main", gs_lat * DEG_TO_RAD, gs_lon * DEG_TO_RAD, gs_alt);

            predict_orbit(satellite_object, &satellite_orbit2, predict_to_julian_double(utc_time));
            predict_observe_orbit(observer_station, &satellite_orbit2, &observation_pos);

            predict_destroy_observer(observer_station);

            return observation_pos;
        }
    };
} // namespace satdump