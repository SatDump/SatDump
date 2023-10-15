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
    std::vector<bool> sat_ascendings;

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
            geodetic::geodetic_coords_t pos_curr = sat_tracker.get_sat_position_at(timestamp);     // Current position
            geodetic::geodetic_coords_t pos_next = sat_tracker.get_sat_position_at(timestamp + 1); // Upcoming position
            sat_positions.push_back(sat_tracker.get_sat_position_at_raw(timestamp));
            sat_ascendings.push_back(pos_curr.lat < pos_next.lat);
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
        bool ascending = sat_ascendings[y];

        if (is_dnb)
        {
                }
        else
        {
            double final_x2 = 0;
            if (final_x < 1280) // First zone
                final_x2 = final_x;
            else if (final_x < 1280 + 736) // Second zone
                final_x2 = 1280 + (final_x - 1280) * 2;
            else if (final_x < 1280 + 736 + 1184) // Third zone
                final_x2 = 1280 + 736 * 2 + (final_x - 1280 - 736) * 3;
            else if (final_x < 1280 + 736 + 1184 + 1184) // Fourth zone
                final_x2 = 1280 + 736 * 2 + 1184 * 3 + (final_x - 1280 - 736 - 1184) * 3;
            else if (final_x < 1280 + 736 + 1184 + 1184 + 736) // Fifth zone
                final_x2 = 1280 + 736 * 2 + 1184 * 3 * 2 + (final_x - 1280 - 736 - 1184 - 1184) * 2;
            else if (final_x < 1280 + 736 + 1184 + 1184 + 736 + 1280) // Fifth zone
                final_x2 = 1280 + 736 * 2 + 1184 * 3 * 2 + 736 * 2 + (final_x - 1280 - 736 - 1184 - 1184 - 736);
            final_x = final_x2;
        }

        double final_width = is_dnb ? 8080 : 12608;

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
