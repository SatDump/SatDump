#pragma once

#include "tle.h"
#include "libs/predict/predict.h"
#include "common/geodetic/geodetic_coordinates.h"

namespace satdump
{
    class SatelliteTracker
    {
    private:
        predict_orbital_elements_t *satellite_object;
        predict_position satellite_orbit;

    public:
        SatelliteTracker(TLE tle);
        ~SatelliteTracker();

        geodetic::geodetic_coords_t get_sat_position_at(double utc_time);
        predict_position get_sat_position_at_raw(double utc_time);
    };
}