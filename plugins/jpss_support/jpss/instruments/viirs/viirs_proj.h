#pragma once

#include "common/projection/sat_proj/sat_proj.h"
#include "common/geodetic/euler_raytrace.h"
#include "nlohmann/json_utils.h"

class VIIRSNormalLineSatProj : public satdump::SatelliteProjection
{
protected:
    std::vector<double> timestamps;

    int image_width;
    float scan_angle;

    double timestamp_offset;
    bool invert_scan;

    float roll_offset;
    float pitch_offset;
    float yaw_offset;

    bool is_dnb;

    // Pre-computed stuff
    std::vector<predict_position> sat_positions;

public:
    VIIRSNormalLineSatProj(nlohmann::ordered_json cfg, satdump::TLE tle, nlohmann::ordered_json timestamps_raw)
        : satdump::SatelliteProjection(cfg, tle, timestamps_raw)
    {
        timestamps = timestamps_raw.get<std::vector<double>>();

        satdump::try_interpolate_timestamps(timestamps, cfg);

        image_width = cfg["image_width"].get<int>();
        scan_angle = cfg["scan_angle"].get<float>();

        gcp_spacing_x = cfg["gcp_spacing_x"].get<int>();
        gcp_spacing_y = cfg["gcp_spacing_y"].get<int>();

        timestamp_offset = getValueOrDefault(cfg["timestamp_offset"], 0.0);
        invert_scan = getValueOrDefault(cfg["invert_scan"], false);

        roll_offset = getValueOrDefault(cfg["roll_offset"], 0.0);
        pitch_offset = getValueOrDefault(cfg["pitch_offset"], 0.0);
        yaw_offset = getValueOrDefault(cfg["yaw_offset"], 0.0);

        is_dnb = getValueOrDefault(cfg["is_dnb"], false);

        img_size_x = image_width;
        img_size_y = timestamps.size();

        for (int i = 0; i < (int)timestamps.size(); i++)
        {
            double timestamp = timestamps[i] + timestamp_offset;
            sat_positions.push_back(sat_tracker.get_sat_position_at_raw(timestamp));
        }
    }

    bool get_position(int x, int y, geodetic::geodetic_coords_t &pos)
    {
        if (x >= image_width)
            return 1;
        if (y >= (int)timestamps.size())
            return 1;

        double timestamp = timestamps[y];

        if (timestamp == -1)
            return 1;

        auto pos_curr = sat_positions[y];

        double final_x = !invert_scan ? (image_width - 1) - x : x;

        // Transform X to undo aggregation zones
        if (!is_dnb)
        {
            std::vector<int> aggr_width = is_dnb
                                              ? std::vector<int>{80, 16, 64, 64, 64, 32, 24, 72, 40, 56, 40, 48, 32, 48, 32, 72, 72, 72, 80, 56, 80, 64, 64, 64, 64, 64, 72, 80, 72, 88, 72, 184,
                                                                 184, 72, 88, 72, 80, 72, 64, 64, 64, 64, 64, 80, 56, 80, 72, 72, 72, 32, 48, 32, 48, 40, 56, 40, 72, 24, 32, 64, 64, 64, 16, 80}
                                              : std::vector<int>{1280, 736, 1184, 1184, 736, 1280};
            std::vector<int> aggr_inter = is_dnb
                                              ? std::vector<int>{/*TODO*/}
                                              : std::vector<int>{1, 2, 3, 3, 2, 1};

            int current_zoff = 0;
            int current_zoff2 = 0;
            int current_zone = 0;
            while (current_zone < 6)
            {
                if ((current_zoff + aggr_width[current_zone]) >= final_x)
                    break;

                current_zoff += aggr_width[current_zone];
                current_zoff2 += aggr_width[current_zone] * aggr_inter[current_zone];
                current_zone++;
            }

            final_x = current_zoff2 + (final_x - current_zoff) * aggr_inter[current_zone];
        }

        double final_width = is_dnb ? 4064 : 12608;

        geodetic::euler_coords_t satellite_pointing;

        satellite_pointing.roll = -(((final_x - (final_width / 2.0)) / final_width) * scan_angle) + roll_offset - 0.06;
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
