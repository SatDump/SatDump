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
    bool is_noaa20; // WIP

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
        is_noaa20 = getValueOrDefault(cfg["is_n20"], false);

        img_size_x = image_width;
        img_size_y = timestamps.size();

        for (int i = 0; i < (int)timestamps.size(); i++)
        {
            double timestamp = timestamps[i] + timestamp_offset;
            sat_positions.push_back(sat_tracker->get_sat_position_at_raw(timestamp));
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

        double final_width = 0;

        // Transform X to undo aggregation zones
        if (is_dnb)
        {
            std::vector<int> aggr_width = {80, 16, 64, 64, 64, 32, 24, 72, 40, 56, 40, 48, 32, 48, 32, 72, 72, 72, 80, 56, 80, 64, 64, 64, 64, 64, 72, 80, 72, 88, 72, 184,
                                           184, 72, 88, 72, 80, 72, 64, 64, 64, 64, 64, 80, 56, 80, 72, 72, 72, 32, 48, 32, 48, 40, 56, 40, 72, 24, 32, 64, 64, 64, 16, 80};
            std::vector<int> aggr_inter = {11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 26, 28, 30, 33, 35, 38, 40, 43, 46, 49, 52, 55, 59, 62, 64, 66};

            std::vector<int> aggr_mode_normal = {32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1,
                                                 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};

            std::vector<int> aggr_mode_jpss1 = {21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1,
                                                2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 32};

            if (is_noaa20)
            {
                aggr_width = {736, 32, 48, 32, 72, 72, 72, 80, 56, 80, 64, 64, 64, 64, 64, 72, 80, 72, 88, 72,
                              368, 72, 88, 72, 80, 72, 64, 64, 64, 64, 64, 80, 56, 80, 72, 72, 72, 32, 48, 32, 456, 8};
            }

            auto &aggr_mode_c = is_noaa20 ? aggr_mode_jpss1 : aggr_mode_normal;

            int all_px = 0, all_px2 = 0;
            for (size_t zone = 0; zone < aggr_mode_c.size(); zone++)
            {
                all_px += aggr_width[zone] * aggr_inter[31 - (aggr_mode_c[zone] - 1)];
                all_px2 += aggr_width[zone];
            }
            // logger->critical("%d %d", all_px, all_px2);

            final_width = is_noaa20 ? /*194464*/ /*191672*/ /*117760*/ all_px : 145040;

            int current_zoff = 0;
            int current_zoff2 = 0;
            int current_zone = 0;
            while (current_zone < (int)aggr_mode_c.size())
            {
                if ((current_zoff + aggr_width[current_zone]) >= final_x)
                    break;

                current_zoff += aggr_width[current_zone];
                current_zoff2 += aggr_width[current_zone] * aggr_inter[31 - (aggr_mode_c[current_zone] - 1)];
                current_zone++;
            }

            final_x = current_zoff2 + (final_x - current_zoff) * aggr_inter[31 - (aggr_mode_c[current_zone] - 1)];
        }
        else
        {
            std::vector<int> aggr_width = {1280, 736, 1184, 1184, 736, 1280};
            std::vector<int> aggr_inter = {1, 2, 3, 3, 2, 1};

            final_width = 12608;

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
