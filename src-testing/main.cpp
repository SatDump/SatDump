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

#include "common/projection/gcp_compute/gcp_compute.h"

#include "common/utils.h"

#include "common/projection/projs/equirectangular.h"

#include "core/config.h"
#include "products/dataset.h"
#include "products/processor/processor.h"

std::vector<satdump::projection::GCP> compute_gcps_normal_per_ifov(nlohmann::ordered_json cfg, satdump::TLE tle, nlohmann::ordered_json timestamps_raw)
{
    std::vector<double> timestamps = timestamps_raw.get<std::vector<double>>();

    int image_width = 1920; // cfg["image_width"].get<int>();
    float scan_angle = 100; // cfg["scan_angle"].get<float>();

    int gcp_spacing_x = 100; // cfg["gcp_spacing_x"].get<int>();
    int gcp_spacing_y = 100; // cfg["gcp_spacing_y"].get<int>();

    double timestamp_offset = 0; // getValueOrDefault(cfg["timestamp_offset"], 0.0);
    bool invert_scan = false;    // getValueOrDefault(cfg["invert_scan"], false);

    float roll_offset = -1.7; // getValueOrDefault(cfg["roll_offset"], 0.0);
    float pitch_offset = 0;   // getValueOrDefault(cfg["pitch_offset"], 0.0);
    float yaw_offset = 0.2;   // getValueOrDefault(cfg["yaw_offset"], 0.0);

    int ifov_y_size = 64;
    int ifov_x_size = 64;
    int ifov_count = 30;

    double ifov_x_scan_angle = 3.4;
    double ifov_y_scan_angle = 3.6;

    std::vector<satdump::projection::GCP> gcps;

    satdump::SatelliteTracker sat_tracker(tle);

    std::vector<int> values;
    for (int x = 0; x < image_width; x += gcp_spacing_x)
        values.push_back(x);
    values.push_back(image_width - 1);

    bool last_was_invalid = false;
    for (int y = 0; y < (timestamps.size() / 30) * 64; y++)
    {
        for (int x : values)
        {
            double final_x = !invert_scan ? (image_width - 1) - x : x;
            int currentScan = y / ifov_y_size;
            int currentIfov = final_x / ifov_x_size;
            int currentArrayValue = currentScan * ifov_count + currentIfov;

            double timestamp = timestamps[currentArrayValue];

            if (timestamp == -1)
            {
                last_was_invalid = true;
                continue;
            }

            timestamp += timestamp_offset;

            geodetic::geodetic_coords_t pos_curr = sat_tracker.get_sat_position_at(timestamp);     // Current position
            geodetic::geodetic_coords_t pos_next = sat_tracker.get_sat_position_at(timestamp + 1); // Upcoming position

            double az_angle = vincentys_inverse(pos_next, pos_curr).reverse_azimuth * RAD_TO_DEG;

            bool ascending = false;

            double currentIfovOffset = -(((double(currentIfov) - (double(ifov_count) / 2)) / double(ifov_count)) * scan_angle);
            double ifov_x = int(final_x) % ifov_x_size;
            double ifov_y = (ifov_y_size - 1) - (y % ifov_y_size);

            geodetic::euler_coords_t satellite_pointing;
            // satellite_pointing.roll = -(((final_x - (image_width / 2.0)) / image_width) * scan_angle) + roll_offset;
            // satellite_pointing.pitch = pitch_offset;
            // satellite_pointing.yaw = (90 + (ascending ? yaw_offset : -yaw_offset)) - az_angle;
            satellite_pointing.roll = -(((ifov_x - (ifov_x_size / 2)) / ifov_x_size) * ifov_x_scan_angle) + currentIfovOffset + roll_offset;
            satellite_pointing.pitch = -(((ifov_y - (ifov_y_size / 2)) / ifov_y_size) * ifov_y_scan_angle) + pitch_offset;
            satellite_pointing.yaw = (90 + (ascending ? yaw_offset : -yaw_offset)) - az_angle;

            geodetic::geodetic_coords_t ground_position;
            geodetic::raytrace_to_earth(pos_curr, satellite_pointing, ground_position);
            ground_position.toDegs();

            // logger->info("{:d} {:d} {:f} {:s}", x, y, pos_curr.lat, img_pro.get_tle().name);

            if (y % gcp_spacing_y == 0 || y + 1 == (int)timestamps.size() || last_was_invalid)
                gcps.push_back({(double)x, (double)y, (double)ground_position.lon, (double)ground_position.lat});

            last_was_invalid = false;
        }
    }

    return gcps;
}

int main(int argc, char *argv[])
{
    initLogger();

    std::string user_path = std::string(getenv("HOME")) + "/.config/satdump";
    satdump::config::loadConfig("satdump_cfg.json", user_path);

    satdump::ImageProducts img_pro;
    img_pro.load(argv[1]);

    nlohmann::json metop_avhrr_cfg;
    // metop_avhrr_cfg.tles = img_pro.get_tle();
    // metop_avhrr_cfg.timestamps = img_pro.get_timestamps(0);
    metop_avhrr_cfg["type"] = "normal_single_line";
    metop_avhrr_cfg["img_width"] = 2048;
    metop_avhrr_cfg["timestamp_offset"] = -0.3;
    metop_avhrr_cfg["scan_angle"] = 110.6;
    metop_avhrr_cfg["roll_offset"] = -0.13;
    metop_avhrr_cfg["gcp_spacing_x"] = 100;
    metop_avhrr_cfg["gcp_spacing_y"] = 100;

    nlohmann::json metop_iasi_img_cfg;
    // metop_mhs_cfg.tles = img_pro.get_tle();
    // metop_mhs_cfg.timestamps = img_pro.get_timestamps(0);
    // metop_mhs_cfg.image_width = 90;
    // metop_mhs_cfg.timestamp_offset = -2;
    // metop_mhs_cfg.scan_angle = 100;
    // metop_mhs_cfg.roll_offset = -1.1;
    // metop_mhs_cfg.gcp_spacing_x = 5;
    // metop_mhs_cfg.gcp_spacing_y = 5;
    metop_iasi_img_cfg["type"] = "normal_single_line";
    metop_iasi_img_cfg["img_width"] = 2048;
    metop_iasi_img_cfg["timestamp_offset"] = -0.3;
    metop_iasi_img_cfg["scan_angle"] = 100;
    metop_iasi_img_cfg["roll_offset"] = -1.7;
    metop_iasi_img_cfg["gcp_spacing_x"] = 100;
    metop_iasi_img_cfg["gcp_spacing_y"] = 100;

    // logger->trace("\n" + img_pro.contents.dump(4));

    std::vector<satdump::projection::GCP> gcps = compute_gcps_normal_per_ifov(metop_iasi_img_cfg, img_pro.get_tle(), img_pro.get_timestamps());

    satdump::ImageCompositeCfg rgb_cfg;
    rgb_cfg.equation = "1-ch1,1-ch1,1-ch1"; //"(ch3 * 0.4 + ch2 * 0.6) * 2.2 - 0.15, ch2 * 2.2 - 0.15, ch1 * 2.2 - 0.15";
    rgb_cfg.equalize = true;

    img_pro.images[0].image.equalize();
    img_pro.images[0].image.to_rgb();

    satdump::warp::WarpOperation operation;
    operation.ground_control_points = gcps;
    operation.input_image = img_pro.images[0].image; // satdump::make_composite_from_product(img_pro, rgb_cfg);
    operation.output_width = 2048 * 8;
    operation.output_height = 1024 * 8;

    satdump::warp::ImageWarper warper;
    warper.op = operation;
    warper.update();

    satdump::warp::WarpResult result = warper.warp();

    logger->info("Drawing map...");

    geodetic::projection::EquirectangularProjection projector;
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
