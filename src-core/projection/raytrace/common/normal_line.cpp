#include "normal_line.h"
#include "common/geodetic/euler_raytrace.h"
#include "nlohmann/json_utils.h"
#include "common/geodetic/vincentys_calculations.h"

namespace satdump
{
    namespace proj
    {
        NormalLineRaytracer::NormalLineRaytracer(nlohmann::json cfg)
            : SatelliteRaytracerSatTrack(cfg)
        {
            timestamps = cfg["timestamps"].get<std::vector<double>>();

            image_width = cfg["image_width"].get<int>();
            scan_angle = cfg["scan_angle"].get<float>();

            // TODOREWORK            gcp_spacing_x = cfg["gcp_spacing_x"].get<int>();
            // TODOREWORK            gcp_spacing_y = cfg["gcp_spacing_y"].get<int>();

            timestamp_offset = getValueOrDefault(cfg["timestamp_offset"], 0.0);
            invert_scan = getValueOrDefault(cfg["invert_scan"], false);
            rotate_yaw = getValueOrDefault(cfg["rotate_yaw"], false);

            roll_offset = getValueOrDefault(cfg["roll_offset"], 0.0);
            pitch_offset = getValueOrDefault(cfg["pitch_offset"], 0.0);
            yaw_offset = getValueOrDefault(cfg["yaw_offset"], 0.0);

            yaw_offset_asc = getValueOrDefault(cfg["yaw_offset_asc"], 0.0);
            yaw_offset_des = getValueOrDefault(cfg["yaw_offset_des"], 0.0);

            // TODOREWORK            img_size_x = image_width;
            // TODOREWORK            img_size_y = timestamps.size();

            for (int i = 0; i < (int)timestamps.size(); i++)
            {
                double timestamp = timestamps[i] + timestamp_offset;
                geodetic::geodetic_coords_t pos_curr = sat_tracker->get_sat_position_at(timestamp);     // Current position
                geodetic::geodetic_coords_t pos_next = sat_tracker->get_sat_position_at(timestamp + 1); // Upcoming position
                sat_positions.push_back(sat_tracker->get_sat_position_at_raw(timestamp));
                sat_ascendings.push_back(pos_curr.lat < pos_next.lat);
            }
        }

        bool NormalLineRaytracer::get_position(double x, double y, geodetic::geodetic_coords_t &pos, double *otime)
        {
            // if (x >= image_width)
            //     return 1;
            if (y >= timestamps.size() || y < 0)
                return 1;

            int iy = floor(y);

            double timestamp = timestamps[iy];

            if (timestamp == -1)
                return 1;
            if (otime != nullptr)
                *otime = timestamp;

            auto pos_curr = sat_positions[iy];
            bool ascending = sat_ascendings[iy];

            geodetic::euler_coords_t satellite_pointing;
            if (rotate_yaw)
            {
                if (yaw_offset_asc != 0 || yaw_offset_des != 0)
                {
                    if (ascending)
                        yaw_offset = yaw_offset_asc;
                    else
                        yaw_offset = yaw_offset_des;
                }
                satellite_pointing.roll = roll_offset;
                satellite_pointing.pitch = pitch_offset;
                satellite_pointing.yaw = yaw_offset + ((invert_scan ? 1.0 : -1.0) * ((x - (image_width / 2.0)) / image_width) * scan_angle);
            }
            else
            {
                satellite_pointing.roll = ((invert_scan ? -1.0 : 1.0) * ((x - (image_width / 2.0)) / image_width) * scan_angle) + roll_offset;
                satellite_pointing.pitch = pitch_offset;
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

        //////////////////////////////////

        NormalLineRaytracerOld::NormalLineRaytracerOld(nlohmann::json cfg)
            : SatelliteRaytracerSatTrack(cfg)
        {
            timestamps = cfg["timestamps"].get<std::vector<double>>();

            image_width = cfg["image_width"].get<int>();
            scan_angle = cfg["scan_angle"].get<float>();

            // TODOREWORK            gcp_spacing_x = cfg["gcp_spacing_x"].get<int>();
            // TODOREWORK            gcp_spacing_y = cfg["gcp_spacing_y"].get<int>();

            timestamp_offset = getValueOrDefault(cfg["timestamp_offset"], 0.0);
            invert_scan = getValueOrDefault(cfg["invert_scan"], false);
            rotate_yaw = getValueOrDefault(cfg["rotate_yaw"], false);

            roll_offset = getValueOrDefault(cfg["roll_offset"], 0.0);
            pitch_offset = getValueOrDefault(cfg["pitch_offset"], 0.0);
            yaw_offset = getValueOrDefault(cfg["yaw_offset"], 0.0);

            yaw_offset_asc = getValueOrDefault(cfg["yaw_offset_asc"], 0.0);
            yaw_offset_des = getValueOrDefault(cfg["yaw_offset_des"], 0.0);

            // TODOREWORK            img_size_x = image_width;
            // TODOREWORK            img_size_y = timestamps.size();

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

        bool NormalLineRaytracerOld::get_position(double x, double y, geodetic::geodetic_coords_t &pos, double *otime)
        {
            // if (x >= image_width)
            //     return 1;
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

            geodetic::euler_coords_t satellite_pointing;
            if (rotate_yaw)
            {
                if (yaw_offset_asc != 0 || yaw_offset_des != 0)
                {
                    if (ascending)
                        yaw_offset = yaw_offset_asc;
                    else
                        yaw_offset = yaw_offset_des;
                }
                satellite_pointing.roll = roll_offset;
                satellite_pointing.pitch = pitch_offset;
                satellite_pointing.yaw = (90 + (!ascending ? yaw_offset : -yaw_offset)) - az_angle + -((invert_scan ? 1.0 : -1.0) * ((x - (image_width / 2.0)) / image_width) * scan_angle);
            }
            else
            {
                satellite_pointing.roll = ((invert_scan ? -1.0 : 1.0) * ((x - (image_width / 2.0)) / image_width) * scan_angle) + roll_offset;
                satellite_pointing.pitch = pitch_offset;
                satellite_pointing.yaw = (90 + (!ascending ? yaw_offset : -yaw_offset)) - az_angle;
            }

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