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
    std::vector<predict_position> sat_positions;
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

        auto pos_curr = sat_positions[currentArrayValue];

        // bool ascending = sat_ascendings[y];

        double currentIfovOffset = -(((double(currentIfov) - (double(ifov_count) / 2)) / double(ifov_count)) * scan_angle);
        if (ifov_count == 1)
            currentIfovOffset = 0;
        double ifov_x = int(final_x) % ifov_x_size;
        double ifov_y = (ifov_y_size - 1) - (y % ifov_y_size);

        geodetic::euler_coords_t satellite_pointing;
        satellite_pointing.roll = -(((ifov_x - (ifov_x_size / 2)) / ifov_x_size) * ifov_x_scan_angle) + currentIfovOffset + roll_offset;
        satellite_pointing.pitch = -(((ifov_y - (ifov_y_size / 2)) / ifov_y_size) * ifov_y_scan_angle) + pitch_offset;
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

class NormalPerIFOVProjOld : public SatelliteProjection
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
    NormalPerIFOVProjOld(nlohmann::ordered_json cfg, TLE tle, nlohmann::ordered_json timestamps_raw)
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
        int ret = geodetic::raytrace_to_earth_old(pos_curr, satellite_pointing, ground_position);
        pos = ground_position.toDegs();

        if (ret)
            return 1;
        else
            return 0;
    }
};