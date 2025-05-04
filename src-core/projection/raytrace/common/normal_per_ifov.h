#pragma once

#include "satellite_raytracer_sattrack.h"

namespace satdump
{
    namespace proj
    {
        class NormalPerIFOVRaytracerOld : public SatelliteRaytracerSatTrack
        {
        private:
            std::vector<double> timestamps;

            int image_width;
            float scan_angle;

            double timestamp_offset;
            bool invert_scan;
            bool rotate_yaw;

            float roll_offset;
            float pitch_offset;
            float yaw_offset;

            int ifov_y_size;
            int ifov_x_size;
            int ifov_count;

            double ifov_x_scan_angle;
            double ifov_y_scan_angle;

            // Pre-computed stuff
            std::vector<geodetic::geodetic_coords_t> sat_positions;
            std::vector<double> az_angles;
            std::vector<bool> sat_ascendings;

        public:
            NormalPerIFOVRaytracerOld(nlohmann::json cfg);
            bool get_position(double x, double y, geodetic::geodetic_coords_t &pos, double *otime);
        };
    } // namespace proj
} // namespace satdump