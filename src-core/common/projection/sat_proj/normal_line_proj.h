
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

    float yaw_offset_asc;
    float yaw_offset_des;

    // Pre-computed stuff
    std::vector<predict_position> sat_positions;
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

        yaw_offset_asc = getValueOrDefault(cfg["yaw_offset_asc"], 0.0);
        yaw_offset_des = getValueOrDefault(cfg["yaw_offset_des"], 0.0);

        img_size_x = image_width;
        img_size_y = timestamps.size();

        for (int i = 0; i < (int)timestamps.size(); i++)
        {
            double timestamp = timestamps[i] + timestamp_offset;
            geodetic::geodetic_coords_t pos_curr = sat_tracker->get_sat_position_at(timestamp);     // Current position
            geodetic::geodetic_coords_t pos_next = sat_tracker->get_sat_position_at(timestamp + 1); // Upcoming position
            sat_positions.push_back(sat_tracker->get_sat_position_at_raw(timestamp));
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
            satellite_pointing.yaw = yaw_offset + (((final_x - (image_width / 2.0)) / image_width) * scan_angle);
        }
        else
        {
            satellite_pointing.roll = -(((final_x - (image_width / 2.0)) / image_width) * scan_angle) + roll_offset - 0.06;
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
};

class NormalLineSatProjOld : public SatelliteProjection
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

    float yaw_offset_asc;
    float yaw_offset_des;

    // Pre-computed stuff
    std::vector<geodetic::geodetic_coords_t> sat_positions;
    std::vector<double> az_angles;
    std::vector<bool> sat_ascendings;

public:
    NormalLineSatProjOld(nlohmann::ordered_json cfg, TLE tle, nlohmann::ordered_json timestamps_raw)
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

        yaw_offset_asc = getValueOrDefault(cfg["yaw_offset_asc"], 0.0);
        yaw_offset_des = getValueOrDefault(cfg["yaw_offset_des"], 0.0);

        img_size_x = image_width;
        img_size_y = timestamps.size();

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
            if (yaw_offset_asc != 0 || yaw_offset_des != 0)
            {
                if (ascending)
                    yaw_offset = yaw_offset_asc;
                else
                    yaw_offset = yaw_offset_des;
            }
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
        int ret = geodetic::raytrace_to_earth_old(pos_curr, satellite_pointing, ground_position);
        pos = ground_position.toDegs();

        if (ret)
            return 1;
        else
            return 0;
    }
};