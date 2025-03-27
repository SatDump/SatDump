#pragma once

#include "satellite_raytracer_sattrack.h"
#include "common/projection/thinplatespline.h"

namespace satdump
{
    namespace proj
    {
        class ManualLineRaytracerOld : public SatelliteRaytracerSatTrack
        {
        private:
            std::vector<double> timestamps;

            int image_width;
            double timestamp_offset;

            projection::VizGeorefSpline2D spline_roll_pitch = projection::VizGeorefSpline2D(2);
            double roll_pitch_v[2] = {0, 0};

            bool yaw_swap;
            double yaw_offset;

            // Pre-computed stuff
            std::vector<geodetic::geodetic_coords_t> sat_positions;
            std::vector<double> az_angles;
            std::vector<bool> sat_ascendings;

        public:
            ManualLineRaytracerOld(nlohmann::json cfg);
            bool get_position(double x, double y, geodetic::geodetic_coords_t &pos, double *otime);
        };
    }
}