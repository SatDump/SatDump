#include "gcp_compute.h"
#include "common/tracking/tracking.h"
#include "common/geodetic/euler_raytrace.h"
#include "common/geodetic/vincentys_calculations.h"
#include "nlohmann/json_utils.h"

namespace satdump
{
    namespace gcp_compute
    {

        std::vector<satdump::projection::GCP> compute_gcps_normal_line(nlohmann::ordered_json cfg, TLE tle, nlohmann::ordered_json timestamps_raw)
        {
            std::vector<double> timestamps = timestamps_raw.get<std::vector<double>>();

            int image_width = cfg["image_width"].get<int>();
            float scan_angle = cfg["scan_angle"].get<float>();

            int gcp_spacing_x = cfg["gcp_spacing_x"].get<int>();
            int gcp_spacing_y = cfg["gcp_spacing_y"].get<int>();

            double timestamp_offset = getValueOrDefault(cfg["timestamp_offset"], 0.0);
            bool invert_scan = getValueOrDefault(cfg["invert_scan"], false);

            float roll_offset = getValueOrDefault(cfg["roll_offset"], 0);
            float pitch_offset = getValueOrDefault(cfg["pitch_offset"], 0);
            float yaw_offset = getValueOrDefault(cfg["yaw_offset"], 0);

            std::vector<satdump::projection::GCP> gcps;

            satdump::SatelliteTracker sat_tracker(tle);

            std::vector<int> values;
            for (int x = 0; x < image_width; x += gcp_spacing_x)
                values.push_back(x);
            values.push_back(image_width - 1);

            int y = 0;
            bool last_was_invalid = false;
            for (double timestamp : timestamps)
            {
                if (timestamp == -1)
                {
                    last_was_invalid = true;
                    continue;
                }

                timestamp += timestamp_offset;

                geodetic::geodetic_coords_t pos_curr = sat_tracker.get_sat_position_at(timestamp);     // Current position
                geodetic::geodetic_coords_t pos_next = sat_tracker.get_sat_position_at(timestamp + 1); // Upcoming position

                double az_angle = vincentys_inverse(pos_next, pos_curr).reverse_azimuth * RAD_TO_DEG;

                for (int x : values)
                {
                    double final_x = !invert_scan ? (image_width - 1) - x : x;
                    bool ascending = false;

                    geodetic::euler_coords_t satellite_pointing;
                    satellite_pointing.roll = -(((final_x - (image_width / 2.0)) / image_width) * scan_angle) + roll_offset;
                    satellite_pointing.pitch = pitch_offset;
                    satellite_pointing.yaw = (90 + (ascending ? yaw_offset : -yaw_offset)) - az_angle;

                    geodetic::geodetic_coords_t ground_position;
                    geodetic::raytrace_to_earth(pos_curr, satellite_pointing, ground_position);
                    ground_position.toDegs();

                    // logger->info("{:d} {:d} {:f} {:s}", x, y, pos_curr.lat, img_pro.get_tle().name);

                    if (y % gcp_spacing_y == 0 || y + 1 == timestamps.size() || last_was_invalid)
                        gcps.push_back({(double)x, (double)y, (double)ground_position.lon, (double)ground_position.lat});

                    last_was_invalid = false;
                }

                y++;
            }

            return gcps;
        }

        std::vector<satdump::projection::GCP> compute_gcps(nlohmann::ordered_json cfg, TLE tle, nlohmann::ordered_json timestamps)
        {
            if (cfg["type"].get<std::string>() == "normal_single_line")
                return compute_gcps_normal_line(cfg, tle, timestamps);
        }
    }
}