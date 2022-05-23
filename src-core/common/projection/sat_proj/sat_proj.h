#pragma once

#include <memory>
#include "common/tracking/tracking.h"
#include "nlohmann/json.hpp"

namespace satdump
{
    class SatelliteProjection
    {
    protected:
        const nlohmann::ordered_json cfg;
        const TLE tle;
        const nlohmann::ordered_json timestamps_raw;
        satdump::SatelliteTracker sat_tracker;

    public:
        SatelliteProjection(nlohmann::ordered_json cfg, TLE tle, nlohmann::ordered_json timestamps_raw)
            : cfg(cfg),
              tle(tle),
              timestamps_raw(timestamps_raw),
              sat_tracker(tle)
        {
        }

        virtual bool get_position(int x, int y, geodetic::geodetic_coords_t &pos) = 0;
    };

    class NormalLineSatProj : public SatelliteProjection
    {
    protected:
        std::vector<double> timestamps;

        int image_width;
        float scan_angle;

        int gcp_spacing_x;
        int gcp_spacing_y;

        double timestamp_offset;
        bool invert_scan;

        float roll_offset;
        float pitch_offset;
        float yaw_offset;

    public:
        NormalLineSatProj(nlohmann::ordered_json cfg, TLE tle, nlohmann::ordered_json timestamps_raw)
            : SatelliteProjection(cfg, tle, timestamps_raw)
        {
            timestamps = timestamps_raw.get<std::vector<double>>();

            image_width = cfg["image_width"].get<int>();
            scan_angle = cfg["scan_angle"].get<float>();

            gcp_spacing_x = cfg["gcp_spacing_x"].get<int>();
            gcp_spacing_y = cfg["gcp_spacing_y"].get<int>();

            timestamp_offset = getValueOrDefault(cfg["timestamp_offset"], 0.0);
            invert_scan = getValueOrDefault(cfg["invert_scan"], false);

            roll_offset = getValueOrDefault(cfg["roll_offset"], 0.0);
            pitch_offset = getValueOrDefault(cfg["pitch_offset"], 0.0);
            yaw_offset = getValueOrDefault(cfg["yaw_offset"], 0.0);
        }

        bool get_position(int x, int y, geodetic::geodetic_coords_t &pos)
        {
            if (x >= image_width)
                return 1;
            if (y >= timestamps.size())
                return 1;

            double timestamp = timestamps[y];

            if (timestamp == -1)
                return 1;

            timestamp += timestamp_offset;

            geodetic::geodetic_coords_t pos_curr = sat_tracker.get_sat_position_at(timestamp);     // Current position
            geodetic::geodetic_coords_t pos_next = sat_tracker.get_sat_position_at(timestamp + 1); // Upcoming position

            double az_angle = vincentys_inverse(pos_next, pos_curr).reverse_azimuth * RAD_TO_DEG;

            double final_x = !invert_scan ? (image_width - 1) - x : x;
            bool ascending = false;

            geodetic::euler_coords_t satellite_pointing;
            satellite_pointing.roll = -(((final_x - (image_width / 2.0)) / image_width) * scan_angle) + roll_offset;
            satellite_pointing.pitch = pitch_offset;
            satellite_pointing.yaw = (90 + (ascending ? yaw_offset : -yaw_offset)) - az_angle;

            geodetic::geodetic_coords_t ground_position;
            int ret = geodetic::raytrace_to_earth(pos_curr, satellite_pointing, ground_position);
            pos = ground_position.toDegs();

            if (ret)
                return 1;
            else
                return 0;
        }
    };

    class NormalPerIFOVProj : public SatelliteProjection
    {
    protected:
        std::vector<double> timestamps;

        int image_width;
        float scan_angle;

        int gcp_spacing_x;
        int gcp_spacing_y;

        double timestamp_offset;
        bool invert_scan;

        float roll_offset;
        float pitch_offset;
        float yaw_offset;

        int ifov_y_size;
        int ifov_x_size;
        int ifov_count;

        double ifov_x_scan_angle;
        double ifov_y_scan_angle;

    public:
        NormalPerIFOVProj(nlohmann::ordered_json cfg, TLE tle, nlohmann::ordered_json timestamps_raw)
            : SatelliteProjection(cfg, tle, timestamps_raw)
        {
            timestamps = timestamps_raw.get<std::vector<double>>();

            image_width = cfg["image_width"].get<int>();
            scan_angle = cfg["scan_angle"].get<float>();

            gcp_spacing_x = cfg["gcp_spacing_x"].get<int>();
            gcp_spacing_y = cfg["gcp_spacing_y"].get<int>();

            timestamp_offset = getValueOrDefault(cfg["timestamp_offset"], 0.0);
            invert_scan = getValueOrDefault(cfg["invert_scan"], false);

            roll_offset = getValueOrDefault(cfg["roll_offset"], 0.0);
            pitch_offset = getValueOrDefault(cfg["pitch_offset"], 0.0);
            yaw_offset = getValueOrDefault(cfg["yaw_offset"], 0.0);

            ifov_y_size = cfg["ifov_y_size"].get<int>();
            ifov_x_size = cfg["ifov_x_size"].get<int>();
            ifov_count = cfg["ifov_count"].get<int>();

            ifov_x_scan_angle = cfg["ifov_x_scan_angle"].get<float>();
            ifov_y_scan_angle = cfg["ifov_y_scan_angle"].get<float>();
        }

        bool get_position(int x, int y, geodetic::geodetic_coords_t &pos)
        {
            if (x >= image_width)
                return 1;
            if (y >= (timestamps.size() / ifov_count) * ifov_y_size)
                return 1;

            double final_x = !invert_scan ? (image_width - 1) - x : x;
            int currentScan = y / ifov_y_size;
            int currentIfov = final_x / ifov_x_size;
            int currentArrayValue = currentScan * ifov_count + currentIfov;

            double timestamp = timestamps[currentArrayValue];

            if (timestamp == -1)
                return 1;

            timestamp += timestamp_offset;

            geodetic::geodetic_coords_t pos_curr = sat_tracker.get_sat_position_at(timestamp);     // Current position
            geodetic::geodetic_coords_t pos_next = sat_tracker.get_sat_position_at(timestamp + 1); // Upcoming position

            double az_angle = vincentys_inverse(pos_next, pos_curr).reverse_azimuth * RAD_TO_DEG;

            bool ascending = false;

            double currentIfovOffset = -(((double(currentIfov) - (double(ifov_count) / 2)) / double(ifov_count)) * scan_angle);
            double ifov_x = int(final_x) % ifov_x_size;
            double ifov_y = (ifov_y_size - 1) - (y % ifov_y_size);

            geodetic::euler_coords_t satellite_pointing;
            satellite_pointing.roll = -(((ifov_x - (ifov_x_size / 2)) / ifov_x_size) * ifov_x_scan_angle) + currentIfovOffset + roll_offset;
            satellite_pointing.pitch = -(((ifov_y - (ifov_y_size / 2)) / ifov_y_size) * ifov_y_scan_angle) + pitch_offset;
            satellite_pointing.yaw = (90 + (ascending ? yaw_offset : -yaw_offset)) - az_angle;

            geodetic::geodetic_coords_t ground_position;
            int ret = geodetic::raytrace_to_earth(pos_curr, satellite_pointing, ground_position);
            pos = ground_position.toDegs();

            if (ret)
                return 1;
            else
                return 0;
        }
    };

    std::shared_ptr<SatelliteProjection> get_sat_proj(nlohmann::ordered_json cfg, TLE tle, nlohmann::ordered_json timestamps_raw)
    {
        if (cfg["type"].get<std::string>() == "normal_single_line")
            return std::make_shared<NormalLineSatProj>(cfg, tle, timestamps_raw);
        else if (cfg["type"].get<std::string>() == "normal_per_ifov")
            return std::make_shared<NormalPerIFOVProj>(cfg, tle, timestamps_raw);
        else
            return std::shared_ptr<SatelliteProjection>();
    }
}