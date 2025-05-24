#pragma once

#include "common/geodetic/euler_raytrace.h"
#include "nlohmann/json_utils.h"
#include "projection/raytrace/common/satellite_raytracer_sattrack.h"

namespace satdump
{
    namespace proj
    {
        class NormalLineXYSatProj : public SatelliteRaytracerSatTrack
        {
        protected:
            std::vector<double> timestamps;

            int image_width, image_height;
            float scan_angle_x, scan_angle_y;

            double timestamp_offset;
            bool invert_scan_x, invert_scan_y;

            float roll_offset;
            float pitch_offset;
            float yaw_offset;

            float yaw_offset_asc;
            float yaw_offset_des;

            // Pre-computed stuff
            std::vector<predict_position> sat_positions;

            double slant_off = 0;
            double slant_sc = 0;

        public:
            NormalLineXYSatProj(nlohmann::ordered_json cfg) : SatelliteRaytracerSatTrack(cfg)
            {
                timestamps = d_cfg["timestamps"].get<std::vector<double>>();

                image_width = cfg["image_width"].get<int>();
                image_height = cfg["image_height"].get<int>();
                scan_angle_x = cfg["scan_angle_x"].get<float>();
                scan_angle_y = cfg["scan_angle_y"].get<float>();

                //                gcp_spacing_x = cfg["gcp_spacing_x"].get<int>();
                //                gcp_spacing_y = cfg["gcp_spacing_y"].get<int>();

                timestamp_offset = getValueOrDefault(cfg["timestamp_offset"], 0.0);
                invert_scan_x = getValueOrDefault(cfg["invert_scan_x"], false);
                invert_scan_y = getValueOrDefault(cfg["invert_scan_y"], false);

                roll_offset = getValueOrDefault(cfg["roll_offset"], 0.0);
                pitch_offset = getValueOrDefault(cfg["pitch_offset"], 0.0);
                yaw_offset = getValueOrDefault(cfg["yaw_offset"], 0.0);

                yaw_offset_asc = getValueOrDefault(cfg["yaw_offset_asc"], 0.0);
                yaw_offset_des = getValueOrDefault(cfg["yaw_offset_des"], 0.0);

                //                img_size_x = image_width;
                //                img_size_y = timestamps.size();

                slant_off = getValueOrDefault(cfg["slant_off"], 0.0);
                slant_sc = getValueOrDefault(cfg["slant_sc"], 0.0);

                for (int i = 0; i < (int)timestamps.size(); i++)
                {
                    double timestamp = timestamps[i] + timestamp_offset;
                    sat_positions.push_back(sat_tracker->get_sat_position_at_raw(timestamp));
                }
            }

            bool get_position(double x, double y, geodetic::geodetic_coords_t &pos, double *otime)
            {
                //                if (x >= image_width)
                //                    return 1;
                if (y >= (int)timestamps.size())
                    return 1;
                if (y >= image_height)
                    return 1;

                int iy = floor(y);

                double timestamp = timestamps[y];

                if (timestamp == -1)
                    return 1;

                auto pos_curr = sat_positions[y];

                // TODO Kind of a hack?
                double added_pitch_off = -pos_curr.latitude * RAD_TO_DEG * 0.1;

                printf("Pitch Off %f\n", added_pitch_off);

                geodetic::euler_coords_t satellite_pointing;

                y += (x / 6004.0) * slant_sc;

                {
                    satellite_pointing.roll = -((((invert_scan_x ? -1.0 : 1.0) * x - (image_width / 2.0)) / image_width) * scan_angle_x) + roll_offset;
                    satellite_pointing.pitch = -((((invert_scan_x ? -1.0 : 1.0) * y - (image_height / 2.0)) / image_height) * scan_angle_y) + pitch_offset + added_pitch_off;
                    satellite_pointing.yaw = yaw_offset;
                }

                geodetic::geodetic_coords_t ground_position;
                int ret = geodetic::raytrace_to_earth(pos_curr.time, pos_curr.position, pos_curr.velocity, satellite_pointing, ground_position);
                pos = ground_position.toDegs();

                if (ret)
                    return 1;
                else
                    return 0;
            }
        };
    } // namespace proj
} // namespace satdump