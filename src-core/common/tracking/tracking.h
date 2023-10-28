#pragma once

#include "tle.h"
#include "libs/predict/predict.h"
#include "common/geodetic/geodetic_coordinates.h"
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
    };
}