#include "sat_proj.h"
#include "common/geodetic/euler_raytrace.h"
#include "common/geodetic/vincentys_calculations.h"
#include "nlohmann/json_utils.h"

#include "common/projection/thinplatespline.h"
#include "logger.h"

#include "common/projection/timestamp_filtering.h"

namespace satdump
{
    void try_interpolate_timestamps(std::vector<double> &timestamps, nlohmann::ordered_json &cfg)
    {
        if (cfg.contains("interpolate_timestamps"))
        {
            int to_interp = cfg["interpolate_timestamps"];
            float scantime = cfg["interpolate_timestamps_scantime"];

            auto timestamp_copy = timestamps;
            timestamps.clear();
            for (auto timestamp : timestamp_copy)
            {
                for (int i = -(to_interp / 2); i < (to_interp / 2); i++)
                {
                    if (timestamp != -1)
                        timestamps.push_back(timestamp + i * scantime);
                    else
                        timestamps.push_back(-1);
                }
            }
        }
    }

    class NormalLineSatProj : public SatelliteProjection
    {
    protected:
        std::vector<double> timestamps;

        int image_width;
        float scan_angle;

        double timestamp_offset;
        bool invert_scan;
        bool rotate_yaw;

        float roll_offset;
        float pitch_offset;
        float yaw_offset;

        // Pre-computed stuff
        std::vector<geodetic::geodetic_coords_t> sat_positions;
        std::vector<double> az_angles;
        std::vector<bool> sat_ascendings;

    public:
        NormalLineSatProj(nlohmann::ordered_json cfg, TLE tle, nlohmann::ordered_json timestamps_raw)
            : SatelliteProjection(cfg, tle, timestamps_raw)
        {
            timestamps = timestamps_raw.get<std::vector<double>>();

            try_interpolate_timestamps(timestamps, cfg);

            image_width = cfg["image_width"].get<int>();
            scan_angle = cfg["scan_angle"].get<float>();

            gcp_spacing_x = cfg["gcp_spacing_x"].get<int>();
            gcp_spacing_y = cfg["gcp_spacing_y"].get<int>();

            timestamp_offset = getValueOrDefault(cfg["timestamp_offset"], 0.0);
            invert_scan = getValueOrDefault(cfg["invert_scan"], false);
            rotate_yaw = getValueOrDefault(cfg["rotate_yaw"], false);

            roll_offset = getValueOrDefault(cfg["roll_offset"], 0.0);
            pitch_offset = getValueOrDefault(cfg["pitch_offset"], 0.0);
            yaw_offset = getValueOrDefault(cfg["yaw_offset"], 0.0);

            img_size_x = image_width;
            img_size_y = timestamps.size();

            for (int i = 0; i < (int)timestamps.size(); i++)
            {
                double timestamp = timestamps[i] + timestamp_offset;
                geodetic::geodetic_coords_t pos_curr = sat_tracker.get_sat_position_at(timestamp);     // Current position
                geodetic::geodetic_coords_t pos_next = sat_tracker.get_sat_position_at(timestamp + 1); // Upcoming position
                double az_angle = vincentys_inverse(pos_next, pos_curr).reverse_azimuth * RAD_TO_DEG;
                sat_positions.push_back(pos_curr);
                az_angles.push_back(az_angle);
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

            geodetic::geodetic_coords_t pos_curr = sat_positions[y];
            double az_angle = az_angles[y];

            double final_x = !invert_scan ? (image_width - 1) - x : x;
            bool ascending = sat_ascendings[y];

            geodetic::euler_coords_t satellite_pointing;
            if (rotate_yaw)
            {
                satellite_pointing.roll = roll_offset;
                satellite_pointing.pitch = pitch_offset;
                satellite_pointing.yaw = (90 + (!ascending ? yaw_offset : -yaw_offset)) - az_angle + -(((final_x - (image_width / 2.0)) / image_width) * scan_angle);
            }
            else
            {
                satellite_pointing.roll = -(((final_x - (image_width / 2.0)) / image_width) * scan_angle) + roll_offset;
                satellite_pointing.pitch = pitch_offset;
                satellite_pointing.yaw = (90 + (!ascending ? yaw_offset : -yaw_offset)) - az_angle;
            }

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

        // Pre-computed stuff
        std::vector<geodetic::geodetic_coords_t> sat_positions;
        std::vector<double> az_angles;
        std::vector<bool> sat_ascendings;

    public:
        NormalPerIFOVProj(nlohmann::ordered_json cfg, TLE tle, nlohmann::ordered_json timestamps_raw)
            : SatelliteProjection(cfg, tle, timestamps_raw)
        {
            timestamps = timestamps_raw.get<std::vector<double>>();

            image_width = cfg["image_width"].get<int>();
            scan_angle = cfg.contains("scan_angle") ? cfg["scan_angle"].get<float>() : (cfg["ifov_x_scan_angle"].get<float>() * cfg["ifov_count"].get<int>());

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

            img_size_x = image_width;
            img_size_y = (timestamps.size() / ifov_count) * ifov_y_size;

            for (int i = 0; i < (int)timestamps.size(); i++)
            {
                double timestamp = timestamps[i] + timestamp_offset;
                geodetic::geodetic_coords_t pos_curr = sat_tracker.get_sat_position_at(timestamp);     // Current position
                geodetic::geodetic_coords_t pos_next = sat_tracker.get_sat_position_at(timestamp + 1); // Upcoming position
                double az_angle = vincentys_inverse(pos_next, pos_curr).reverse_azimuth * RAD_TO_DEG;
                sat_positions.push_back(pos_curr);
                az_angles.push_back(az_angle);
                sat_ascendings.push_back(pos_curr.lat < pos_next.lat);
            }
        }

        bool get_position(int x, int y, geodetic::geodetic_coords_t &pos)
        {
            if (x >= image_width)
                return 1;
            if (y >= int(timestamps.size() / ifov_count) * ifov_y_size)
                return 1;

            double final_x = !invert_scan ? (image_width - 1) - x : x;
            int currentScan = y / ifov_y_size;
            int currentIfov = final_x / ifov_x_size;
            int currentArrayValue = currentScan * ifov_count + currentIfov;

            double timestamp = timestamps[currentArrayValue];

            if (timestamp == -1)
                return 1;

            timestamp += timestamp_offset;

            geodetic::geodetic_coords_t pos_curr = sat_positions[currentArrayValue];
            double az_angle = az_angles[currentArrayValue];

            bool ascending = sat_ascendings[y];

            double currentIfovOffset = -(((double(currentIfov) - (double(ifov_count) / 2)) / double(ifov_count)) * scan_angle);
            if (ifov_count == 1)
                currentIfovOffset = 0;
            double ifov_x = int(final_x) % ifov_x_size;
            double ifov_y = (ifov_y_size - 1) - (y % ifov_y_size);

            geodetic::euler_coords_t satellite_pointing;
            satellite_pointing.roll = -(((ifov_x - (ifov_x_size / 2)) / ifov_x_size) * ifov_x_scan_angle) + currentIfovOffset + roll_offset;
            satellite_pointing.pitch = -(((ifov_y - (ifov_y_size / 2)) / ifov_y_size) * ifov_y_scan_angle) + pitch_offset;
            satellite_pointing.yaw = (90 + (!ascending ? yaw_offset : -yaw_offset)) - az_angle;

            geodetic::geodetic_coords_t ground_position;
            int ret = geodetic::raytrace_to_earth(pos_curr, satellite_pointing, ground_position);
            pos = ground_position.toDegs();

            if (ret)
                return 1;
            else
                return 0;
        }
    };

    class NormalLineManualSatProj : public SatelliteProjection
    {
    protected:
        std::vector<double> timestamps;

        int image_width;
        double timestamp_offset;

        // Pre-computed stuff
        std::vector<geodetic::geodetic_coords_t> sat_positions;
        std::vector<double> az_angles;
        std::vector<bool> sat_ascendings;

        double yaw_offset;
#if 0
        projection::VizGeorefSpline2D spline_roll;
        projection::VizGeorefSpline2D spline_pitch;
        projection::VizGeorefSpline2D spline_yaw;

        double roll_v[2] = {0, 0};
        double pitch_v[2] = {0, 0};
        double yaw_v[2] = {0, 0};
#endif
        projection::VizGeorefSpline2D spline_roll_pitch = projection::VizGeorefSpline2D(2);
        double roll_pitch_v[2] = {0, 0};

        bool yaw_swap;

    public:
        NormalLineManualSatProj(nlohmann::ordered_json cfg, TLE tle, nlohmann::ordered_json timestamps_raw)
            : SatelliteProjection(cfg, tle, timestamps_raw)
        {
            timestamps = timestamps_raw.get<std::vector<double>>();

            try_interpolate_timestamps(timestamps, cfg);

            image_width = cfg["image_width"].get<int>();
            gcp_spacing_x = cfg["gcp_spacing_x"].get<int>();
            gcp_spacing_y = cfg["gcp_spacing_y"].get<int>();

            yaw_offset = getValueOrDefault(cfg["yaw_offset"], 0.0);

            timestamp_offset = getValueOrDefault(cfg["timestamp_offset"], 0.0);

            yaw_swap = getValueOrDefault(cfg["yaw_swap"], true);

#if 0
            for (auto item : cfg["points"].items())
            {
                int px = std::stoi(item.key());
                double roll = item.value()[0].get<double>();
                double pitch = item.value()[1].get<double>();
                double yaw = item.value()[2].get<double>();

                double r_v[2] = {roll, 0};
                double p_v[2] = {pitch, 0};
                double y_v[2] = {yaw, 0};
                spline_roll.add_point(px, 0, r_v);
                spline_pitch.add_point(px, 0, p_v);
                spline_yaw.add_point(px, 0, y_v);

                logger->critical("{:d} {:f} {:f} {:f} ", px, roll, pitch, yaw);
            }

            spline_roll.solve();
            spline_pitch.solve();
            spline_yaw.solve();
#endif

            for (auto item : cfg["points"].items())
            {
                int px = std::stoi(item.key());
                double roll = item.value()[0].get<double>();
                double pitch = item.value()[1].get<double>();
                // double yaw = item.value()[2].get<double>();

                double rp_v[2] = {roll, pitch};
                spline_roll_pitch.add_point(px, px, rp_v);

                // logger->critical("{:d} {:f} {:f} {:f} ", px, roll, pitch, yaw);
            }

            spline_roll_pitch.solve();
            logger->info("SOLVED PROJ");

            img_size_x = image_width;
            img_size_y = timestamps.size();

            for (int i = 0; i < (int)timestamps.size(); i++)
            {
                double timestamp = timestamps[i] + timestamp_offset;
                geodetic::geodetic_coords_t pos_curr = sat_tracker.get_sat_position_at(timestamp);     // Current position
                geodetic::geodetic_coords_t pos_next = sat_tracker.get_sat_position_at(timestamp + 1); // Upcoming position
                double az_angle = vincentys_inverse(pos_next, pos_curr).reverse_azimuth * RAD_TO_DEG;
                sat_positions.push_back(pos_curr);
                az_angles.push_back(az_angle);
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

            geodetic::geodetic_coords_t pos_curr = sat_positions[y];
            double az_angle = az_angles[y];

            bool ascending = sat_ascendings[y];

#if 0
            spline_roll.get_point(x, 0, roll_v);
            spline_pitch.get_point(x, 0, pitch_v);
            spline_yaw.get_point(x, 0, yaw_v);

            geodetic::euler_coords_t satellite_pointing;
            satellite_pointing.roll = roll_v[0];
            satellite_pointing.pitch = pitch_v[0];
            satellite_pointing.yaw = 90 + yaw_v[0] - az_angle;
#endif

            spline_roll_pitch.get_point(x, x, roll_pitch_v);

            geodetic::euler_coords_t satellite_pointing;
            satellite_pointing.roll = roll_pitch_v[0];
            satellite_pointing.pitch = roll_pitch_v[1];
            satellite_pointing.yaw = 90 + (yaw_swap ? (!ascending ? yaw_offset : -yaw_offset) : yaw_offset) - az_angle;

            // logger->critical("{:f} {:f} {:f} ", satellite_pointing.roll, satellite_pointing.pitch, satellite_pointing.yaw);

            geodetic::geodetic_coords_t ground_position;
            int ret = geodetic::raytrace_to_earth(pos_curr, satellite_pointing, ground_position);
            pos = ground_position.toDegs();

            if (ret)
                return 1;
            else
                return 0;
        }
    };

    std::shared_ptr<SatelliteProjection> get_sat_proj(nlohmann::ordered_json cfg, TLE tle, std::vector<double> timestamps_raw)
    {
        if (cfg.contains("timefilter"))
            timestamps_raw = timestamp_filtering::filter_timestamps_width_cfg(timestamps_raw, cfg["timefilter"]);

        if (cfg["type"].get<std::string>() == "normal_single_line")
            return std::make_shared<NormalLineSatProj>(cfg, tle, timestamps_raw);
        else if (cfg["type"].get<std::string>() == "normal_per_ifov")
            return std::make_shared<NormalPerIFOVProj>(cfg, tle, timestamps_raw);
        else if (cfg["type"].get<std::string>() == "manual_single_line")
            return std::make_shared<NormalLineManualSatProj>(cfg, tle, timestamps_raw);
        throw std::runtime_error("Invalid satellite projection!");
    }
}