/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"
#include "common/image/image.h"

#include "products/image_products.h"
#include "common/tracking/tracking.h"
#include "common/geodetic/euler_raytrace.h"
#include "common/geodetic/vincentys_calculations.h"
#include "common/map/map_drawer.h"
#include "resources.h"

#include <iostream>
#include "common/projection/warp/warp.h"
#include "core/config.h"

struct SingleLineCfg
{
    satdump::TLE tles;
    std::vector<double> timestamps;

    int image_width;
    float scan_angle;

    int gcp_spacing_x = 100;
    int gcp_spacing_y = 100;

    double timestamp_offset = 0;
    bool invert_scan = false;

    float roll_offset = 0;
    float pitch_offset = 0;
    float yaw_offset = 0;
};

std::vector<satdump::projection::GCP> compute_gcps(SingleLineCfg cfg)
{
    std::vector<satdump::projection::GCP> gcps;

    satdump::SatelliteTracker sat_tracker(cfg.tles);

    std::vector<int> values;
    for (int x = 0; x < cfg.image_width; x += cfg.gcp_spacing_x)
        values.push_back(x);
    values.push_back(cfg.image_width - 1);

    int y = 0;
    bool last_was_invalid = false;
    for (double timestamp : cfg.timestamps)
    {
        if (timestamp == -1)
        {
            last_was_invalid = true;
            continue;
        }

        timestamp += cfg.timestamp_offset;

        geodetic::geodetic_coords_t pos_curr = sat_tracker.get_sat_position_at(timestamp);     // Current position
        geodetic::geodetic_coords_t pos_next = sat_tracker.get_sat_position_at(timestamp + 1); // Upcoming position

        double az_angle = vincentys_inverse(pos_next, pos_curr).reverse_azimuth * RAD_TO_DEG;

        for (int x : values)
        {
            double final_x = !cfg.invert_scan ? (cfg.image_width - 1) - x : x;
            bool ascending = false;

            geodetic::euler_coords_t satellite_pointing;
            satellite_pointing.roll = -(((final_x - (cfg.image_width / 2.0)) / cfg.image_width) * cfg.scan_angle) + cfg.roll_offset;
            satellite_pointing.pitch = cfg.pitch_offset;
            satellite_pointing.yaw = (90 + (ascending ? cfg.yaw_offset : -cfg.yaw_offset)) - az_angle;

            geodetic::geodetic_coords_t ground_position;
            geodetic::raytrace_to_earth(pos_curr, satellite_pointing, ground_position);
            ground_position.toDegs();

            // logger->info("{:d} {:d} {:f} {:s}", x, y, pos_curr.lat, img_pro.get_tle().name);

            if (y % cfg.gcp_spacing_y == 0 || y + 1 == cfg.timestamps.size() || last_was_invalid)
                gcps.push_back({(double)x, (double)y, (double)ground_position.lon, (double)ground_position.lat});

            last_was_invalid = false;
        }

        y++;
    }

    return gcps;
}

class EquirectangularProjector
{
private:
    int image_height;
    int image_width;

    float top_left_lat;
    float top_left_lon;

    float bottom_right_lat;
    float bottom_right_lon;

    float covered_lat;
    float covered_lon;

    float offset_lat;
    float offset_lon;

public:
    void init(int img_width, int img_height, float tl_lon, float tl_lat, float br_lon, float br_lat)
    {
        image_height = img_height;
        image_width = img_width;

        top_left_lat = tl_lat;
        top_left_lon = tl_lon;

        bottom_right_lat = br_lat;
        bottom_right_lon = br_lon;

        // Compute how much we cover on the input image
        covered_lat = abs(top_left_lat - bottom_right_lat);
        covered_lon = abs(top_left_lon - bottom_right_lon);

        // Compute the offset the top right corner has
        offset_lat = abs(top_left_lat - 90);
        offset_lon = abs(top_left_lon + 180);
    }

    void forward(float lon, float lat, int &x, int &y)
    {
        if (lat > top_left_lat || lat < bottom_right_lat || lon < top_left_lon || lon > bottom_right_lon)
        {
            x = y = -1;
            return;
        }

        lat = 180.0f - (lat + 90.0f);
        lon += 180;

        lat -= offset_lat;
        lon -= offset_lon;

        y = (lat / covered_lat) * image_height;
        x = (lon / covered_lon) * image_width;

        if (y < 0 || y >= image_height || x < 0 || x >= image_width)
            x = y = -1;
    }

    void reverse(int x, int y, float &lon, float &lat)
    {
        if (y < 0 || y >= image_height || x < 0 || x >= image_width)
        {
            lon = lat = -1;
            return;
        }

        lat = (y / (float)image_height) * covered_lat;
        lon = (x / (float)image_width) * covered_lon;

        lat += offset_lat;
        lon += offset_lon;

        lat = 180.0f - (lat + 90.0f);
        lon -= 180;

        if (lat > top_left_lat || lat < bottom_right_lat || lon < top_left_lon || lon > bottom_right_lon)
            lon = lat = -1;
    }
};

int main(int argc, char *argv[])
{
    initLogger();
    std::string user_path = std::string(getenv("HOME")) + "/.config/satdump";
    satdump::config::loadConfig("satdump_cfg.json", user_path);

    satdump::ImageProducts img_pro;
    img_pro.load(argv[1]);

    SingleLineCfg metop_avhrr_cfg;
    metop_avhrr_cfg.tles = img_pro.get_tle();
    metop_avhrr_cfg.timestamps = img_pro.get_timestamps(0);
    metop_avhrr_cfg.image_width = 2048;
    metop_avhrr_cfg.timestamp_offset = -0.3;
    metop_avhrr_cfg.scan_angle = 110.6;
    metop_avhrr_cfg.roll_offset = -0.13;
    metop_avhrr_cfg.gcp_spacing_x = 100;
    metop_avhrr_cfg.gcp_spacing_y = 100;

    SingleLineCfg metop_mhs_cfg;
    metop_mhs_cfg.tles = img_pro.get_tle();
    metop_mhs_cfg.timestamps = img_pro.get_timestamps(0);
    metop_mhs_cfg.image_width = 90;
    metop_mhs_cfg.timestamp_offset = -2;
    metop_mhs_cfg.scan_angle = 100;
    metop_mhs_cfg.roll_offset = -1.1;
    metop_mhs_cfg.gcp_spacing_x = 5;
    metop_mhs_cfg.gcp_spacing_y = 5;

    std::vector<satdump::projection::GCP> gcps = compute_gcps(metop_avhrr_cfg);

    satdump::ImageCompositeCfg rgb_cfg;
    rgb_cfg.equation = "(ch3 * 0.4 + ch2 * 0.6) * 2.2 - 0.15, ch2 * 2.2 - 0.15, ch1 * 2.2 - 0.15";
    // rgb_cfg.equalize = true;

    satdump::warp::WarpOperation operation;
    operation.ground_control_points = gcps;
    operation.input_image = satdump::make_composite_from_product(img_pro, rgb_cfg);
    operation.output_width = 2048 * 8;
    operation.output_height = 1024 * 8;

    satdump::warp::ImageWarper warper;
    warper.op = operation;
    warper.update();

    satdump::warp::WarpResult result = warper.warp();

    logger->info("Drawing map...");

    EquirectangularProjector projector;
    projector.init(result.output_image.width(), result.output_image.height(), result.top_left.lon, result.top_left.lat, result.bottom_right.lon, result.bottom_right.lat);

    unsigned short color[3] = {0, 65535, 0};
    map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                   result.output_image,
                                   color,
                                   [operation, &result, &projector](float lat, float lon, int map_height2, int map_width2) -> std::pair<int, int>
                                   {
                                       int x, y;
                                       projector.forward(lon, lat, x, y);
                                       return {x, y};
                                   });

    // img_map.crop(p_x_min, p_y_min, p_x_max, p_y_max);
    logger->info("Saving...");

    result.output_image.save_png("test.png");
}