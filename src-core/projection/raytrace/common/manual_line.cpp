#include "manual_line.h"
#include "common/geodetic/euler_raytrace.h"
#include "nlohmann/json_utils.h"
#include "common/geodetic/vincentys_calculations.h"

namespace satdump
{
    namespace proj
    {
        ManualLineRaytracerOld::ManualLineRaytracerOld(nlohmann::json cfg)
            : SatelliteRaytracerSatTrack(cfg)
        {
            timestamps = cfg["timestamps"].get<std::vector<double>>();

            image_width = cfg["image_width"].get<int>();

            yaw_offset = getValueOrDefault(cfg["yaw_offset"], 0.0);

            timestamp_offset = getValueOrDefault(cfg["timestamp_offset"], 0.0);

            yaw_swap = getValueOrDefault(cfg["yaw_swap"], true);

            for (auto item : cfg["points"].items())
            {
                int px = std::stoi(item.key());
                double roll = item.value()[0].get<double>();
                double pitch = item.value()[1].get<double>();
                // double yaw = item.value()[2].get<double>();

                double rp_v[2] = {roll, pitch};
                spline_roll_pitch.add_point(px, px, rp_v);

                // logger->critical("%d %f %f %f ", px, roll, pitch, yaw);
            }

            spline_roll_pitch.solve();

            for (int i = 0; i < (int)timestamps.size(); i++)
            {
                double timestamp = timestamps[i] + timestamp_offset;
                geodetic::geodetic_coords_t pos_curr = sat_tracker->get_sat_position_at(timestamp);     // Current position
                geodetic::geodetic_coords_t pos_next = sat_tracker->get_sat_position_at(timestamp + 1); // Upcoming position
                double az_angle = vincentys_inverse(pos_next, pos_curr).reverse_azimuth * RAD_TO_DEG;
                sat_positions.push_back(pos_curr);
                az_angles.push_back(az_angle);
                sat_ascendings.push_back(pos_curr.lat < pos_next.lat);
            }
        }

        bool ManualLineRaytracerOld::get_position(double x, double y, geodetic::geodetic_coords_t &pos, double *otime)
        {
            if (x >= image_width)
                return 1;
            if (y >= (int)timestamps.size() || y < 0)
                return 1;

            int iy = floor(y);

            double timestamp = timestamps[iy];

            if (timestamp == -1)
                return 1;
            if (otime != nullptr)
                *otime = timestamp;

            geodetic::geodetic_coords_t pos_curr = sat_positions[iy];
            double az_angle = az_angles[iy];

            bool ascending = sat_ascendings[y];

            spline_roll_pitch.get_point(x, x, roll_pitch_v);

            geodetic::euler_coords_t satellite_pointing;
            satellite_pointing.roll = roll_pitch_v[0];
            satellite_pointing.pitch = roll_pitch_v[1];
            satellite_pointing.yaw = 90 + (yaw_swap ? (!ascending ? yaw_offset : -yaw_offset) : yaw_offset) - az_angle;

            // logger->critical("%f %f %f ", satellite_pointing.roll, satellite_pointing.pitch, satellite_pointing.yaw);

            geodetic::geodetic_coords_t ground_position;
            int ret = geodetic::raytrace_to_earth_old(pos_curr, satellite_pointing, ground_position);
            pos = ground_position.toDegs();

            if (ret)
                return 1;
            else
                return 0;
        }
    }
}