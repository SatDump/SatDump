#pragma once

#include "projection/raytrace/common/satellite_raytracer_sattrack.h"
#include "common/geodetic/euler_raytrace.h"
#include "nlohmann/json_utils.h"

namespace noaa_apt
{
    class NOAA_APT_SatProj : public satdump::proj::SatelliteRaytracerSatTrack
    {
    protected:
        std::vector<double> timestamps;

        float scan_angle;

        double timestamp_offset;

        float roll_offset;
        float pitch_offset;
        float yaw_offset;

        // Pre-computed stuff
        std::vector<predict_position> sat_positions;

    public:
        NOAA_APT_SatProj(nlohmann::json cfg)
            : satdump::proj::SatelliteRaytracerSatTrack(cfg)
        {
            timestamps = cfg["timestamps"].get<std::vector<double>>();

            scan_angle = cfg["scan_angle"].get<float>();

            timestamp_offset = getValueOrDefault(cfg["timestamp_offset"], 0.0);

            roll_offset = getValueOrDefault(cfg["roll_offset"], 0.0);
            pitch_offset = getValueOrDefault(cfg["pitch_offset"], 0.0);
            yaw_offset = getValueOrDefault(cfg["yaw_offset"], 0.0);

            for (int i = 0; i < (int)timestamps.size(); i++)
            {
                double timestamp = timestamps[i] + timestamp_offset;
                sat_positions.push_back(sat_tracker->get_sat_position_at_raw(timestamp));
            }
        }

        bool get_position(double x, double y, geodetic::geodetic_coords_t &pos, double *otime)
        {
            if (y >= (int)timestamps.size())
                return 1;

            int iy = floor(y);

            double timestamp = timestamps[iy];

            if (timestamp == -1)
                return 1;

            if (otime != nullptr)
                *otime = timestamp;

            auto pos_curr = sat_positions[iy];

            double final_width = 2048;

            {
                std::vector<double> aggr_width = {121, 62, 83, 110, 157, 110, 83, 62, 121};
                std::vector<double> aggr_inter = {1, 1.5, 2, 3, 4, 3, 2, 1.5, 1};

                int current_zoff = 0;
                int current_zoff2 = 0;
                int current_zone = 0;
                while (current_zone < 10)
                {
                    if ((current_zoff + aggr_width[current_zone]) >= x)
                        break;

                    current_zoff += aggr_width[current_zone];
                    current_zoff2 += aggr_width[current_zone] * aggr_inter[current_zone];
                    current_zone++;
                }

                x = current_zoff2 + (x - current_zoff) * aggr_inter[current_zone];
            }

            geodetic::euler_coords_t satellite_pointing;

            satellite_pointing.roll = (((x - (final_width / 2.0)) / final_width) * scan_angle) + roll_offset;
            satellite_pointing.pitch = pitch_offset;
            satellite_pointing.yaw = yaw_offset;

            geodetic::geodetic_coords_t ground_position;
            int ret = geodetic::raytrace_to_earth(pos_curr.time, pos_curr.position, pos_curr.velocity, satellite_pointing, ground_position);
            pos = ground_position.toDegs();

            if (ret)
                return 1;
            else
                return 0;
        }
    };
}