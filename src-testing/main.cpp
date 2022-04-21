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

int main(int argc, char *argv[])
{
    initLogger();

    satdump::ImageProducts img_pro;
    img_pro.load("/home/alan/Documents/SatDump_ReWork/build/metop_ahrpt_3/AVHRR/product.cbor");

    std::vector<satdump::projection::GCP> gcps;

#if 1
    {
        satdump::SatelliteTracker sat_tracker(img_pro.get_tle());

        std::vector<int> values;

        for (int x = 0; x < 2048; x += 100)
            values.push_back(x);

        values.push_back(2047);

        int y = 0;
        for (double timestamp : img_pro.contents["timestamps"].get<std::vector<double>>())
        {
            timestamp -= 0.3;

            geodetic::geodetic_coords_t pos_curr = sat_tracker.get_sat_position_at(timestamp);     // Current position
            geodetic::geodetic_coords_t pos_next = sat_tracker.get_sat_position_at(timestamp + 1); // Upcoming position

            double az_angle = vincentys_inverse(pos_next, pos_curr).reverse_azimuth * RAD_TO_DEG;

            for (int x : values)
            {
                bool invert_scan = true;

                float roll_offset = -0.13;
                float pitch_offset = 0;
                float yaw_offset = 0;
                float scan_angle = 110.6;

                double final_x = invert_scan ? (2048 - 1) - x : x;
                bool ascending = false;

                geodetic::euler_coords_t satellite_pointing;
                satellite_pointing.roll = -(((final_x - (2048.0 / 2.0)) / 2048.0) * scan_angle) + roll_offset;
                satellite_pointing.pitch = pitch_offset;
                satellite_pointing.yaw = (90 + (ascending ? yaw_offset : -yaw_offset)) - az_angle;

                geodetic::geodetic_coords_t ground_position;
                geodetic::raytrace_to_earth(pos_curr, satellite_pointing, ground_position);
                ground_position.toDegs();

                // logger->info("{:d} {:d} {:f} {:s}", x, y, pos_curr.lat, img_pro.get_tle().name);

                if (y % 100 == 0 || y + 1 == img_pro.contents["timestamps"].get<std::vector<double>>().size())
                    gcps.push_back({x, y, ground_position.lon, ground_position.lat});
            }

            y++;
        }
    }
#else
    {
        satdump::SatelliteTracker sat_tracker(img_pro.get_tle());

        std::vector<int> values;

        for (int x = 0; x < 90; x += 5)
            values.push_back(x);

        values.push_back(90 - 1);

        int y = 0;
        for (double timestamp : img_pro.contents["timestamps"].get<std::vector<double>>())
        {
            timestamp -= 2;

            geodetic::geodetic_coords_t pos_curr = sat_tracker.get_sat_position_at(timestamp);     // Current position
            geodetic::geodetic_coords_t pos_next = sat_tracker.get_sat_position_at(timestamp + 1); // Upcoming position

            double az_angle = vincentys_inverse(pos_next, pos_curr).reverse_azimuth * RAD_TO_DEG;

            for (int x : values)
            {
                bool invert_scan = true;

                float roll_offset = -1.1;
                float pitch_offset = 0;
                float yaw_offset = 0;
                float scan_angle = 100;

                double final_x = invert_scan ? (90 - 1) - x : x;
                bool ascending = false;

                geodetic::euler_coords_t satellite_pointing;
                satellite_pointing.roll = -(((final_x - (90.0 / 2.0)) / 90.0) * scan_angle) + roll_offset;
                satellite_pointing.pitch = pitch_offset;
                satellite_pointing.yaw = (90 + (ascending ? yaw_offset : -yaw_offset)) - az_angle;

                geodetic::geodetic_coords_t ground_position;
                geodetic::raytrace_to_earth(pos_curr, satellite_pointing, ground_position);
                ground_position.toDegs();

                // logger->info("{:d} {:d} {:f} {:s}", x, y, pos_curr.lat, img_pro.get_tle().name);

                if (y % 5 == 0 || y + 1 == img_pro.contents["timestamps"].get<std::vector<double>>().size())
                    gcps.push_back({x, y, ground_position.lon, ground_position.lat});
            }

            y++;
        }
    }
#endif

    satdump::ImageCompositeCfg rgb_cfg;
    rgb_cfg.equation = "ch1, ch2, ch4"; //*/ "(ch3 * 0.4 + ch2 * 0.6) * 2.2 - 0.15, ch2 * 2.2 - 0.15, ch1 * 2.2 - 0.15";
    rgb_cfg.equalize = true;

    satdump::warp::WarpOperation operation;
    operation.ground_control_points = gcps;
    operation.input_image = satdump::make_composite_from_product(img_pro, rgb_cfg);
    operation.output_width = 2048 * 8;
    operation.output_height = 1024 * 8;

    satdump::warp::WarpCropSettings crop_set = choseCropArea(operation);

    satdump::warp::WarpResult result = satdump::warp::warpOnAvailable(operation);

    logger->info("Drawing map...");

    unsigned short color[3] = {0, 65535, 0};
    map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                   result.output_image,
                                   color,
                                   [operation, &result, crop_set](float lat, float lon, int map_height2, int map_width2) -> std::pair<int, int>
                                   {
#if 0
                                       double covered_lat = fabs(result.top_left.lat - result.bottom_left.lat);
                                       double covered_lon = fabs(result.top_left.lon - result.top_right.lon);

                                       double total_height = ceil((180.0 / covered_lat) * map_height2);
                                       double total_width = ceil((360.0 / covered_lon) * map_width2);

                                       double imageLat = total_height - ((90.0f + lat) / 180.0f) * total_height;
                                       double imageLon = (lon / 360.0f) * total_width + (total_width / 2);

                                       imageLat -= total_height - ((90.0f + result.bottom_left.lat) / 180.0f) * total_height;
                                       imageLon -= (result.top_left.lon / 360.0f) * total_width + (total_width / 2);
#else
            int imageLat = operation.output_height - ((90.0f + lat) / 180.0f) * operation.output_height;
            int imageLon = (lon / 360.0f) * operation.output_width + (operation.output_width / 2);

            imageLat -= crop_set.y_min;
            imageLon -= crop_set.x_min;
#endif
                                       if (imageLat < 0 || imageLat > map_height2)
                                           return {-1, -1};
                                       if (imageLon < 0 || imageLon > map_width2)
                                           return {-1, -1};

                                       return {imageLon, imageLat};
                                   });

    // img_map.crop(p_x_min, p_y_min, p_x_max, p_y_max);
    logger->info("Saving...");

    result.output_image.save_png("test.png");
}