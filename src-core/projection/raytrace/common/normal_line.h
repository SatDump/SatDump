#pragma once

#include "satellite_raytracer_sattrack.h"

namespace satdump
{
    namespace proj
    {
        class NormalLineRaytracer : public SatelliteRaytracerSatTrack
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

            float yaw_offset_asc;
            float yaw_offset_des;

            // Pre-computed stuff
            std::vector<predict_position> sat_positions;
            std::vector<bool> sat_ascendings;

        public:
            NormalLineRaytracer(nlohmann::json cfg);
            bool get_position(double x, double y, geodetic::geodetic_coords_t &pos);
        };

        class NormalLineRaytracerOld : public SatelliteRaytracerSatTrack
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

            float yaw_offset_asc;
            float yaw_offset_des;

            // Pre-computed stuff
            std::vector<geodetic::geodetic_coords_t> sat_positions;
            std::vector<double> az_angles;
            std::vector<bool> sat_ascendings;

        public:
            NormalLineRaytracerOld(nlohmann::json cfg);
            bool get_position(double x, double y, geodetic::geodetic_coords_t &pos);
        };
    }
}