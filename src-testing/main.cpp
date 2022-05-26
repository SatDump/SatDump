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

#include "common/projection/sat_proj/sat_proj.h"

int main(int argc, char *argv[])
{
#if 0
    image::Image<uint16_t> mersi_ch1, mersi_ch2;
    mersi_ch1.load_png("/home/alan/Documents/SatDump_ReWork/build/fy3_mersi_offset/MERSI-1/MERSI1-1.png");
    mersi_ch2.load_png("/home/alan/Documents/SatDump_ReWork/build/fy3_mersi_offset/MERSI-1/MERSI1-20.png");

    mersi_ch2.resize(mersi_ch2.width() * 4, mersi_ch2.height() * 4);

    mersi_ch1.equalize();
    mersi_ch2.equalize();

    image::Image<uint16_t> mersi_rgb(mersi_ch1.width(), mersi_ch1.height(), 3);

    mersi_rgb.draw_image(0, mersi_ch1, 0);
    mersi_rgb.draw_image(1, mersi_ch1, 0);
    mersi_rgb.draw_image(2, mersi_ch2, -24);

    mersi_rgb.save_png("mersi_test.png");

    return 0; // TMP
#endif

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

    // img_pro.images[3].image.equalize();

    // std::shared_ptr<satdump::SatelliteProjection> satellite_proj = satdump::get_sat_proj(img_pro.get_proj_cfg(), img_pro.get_tle(), img_pro.get_timestamps(0));
    // geodetic::projection::EquirectangularProjection projector_target;
    // projector_target.init(2048 * 4, 1024 * 4, -180, 90, 180, -90);

#if 0
    image::Image<uint16_t> final_image(2048 * 4, 1024 * 4, 3);

    int radius = 0;

    for (int img_y = 0; img_y < img_pro.images[0].image.height(); img_y++)
    {
        for (int img_x = 0; img_x < img_pro.images[0].image.width(); img_x++)
        {
            geodetic::geodetic_coords_t position;
            geodetic::geodetic_coords_t position2, position3;
            if (!satellite_proj->get_position(img_x, img_y, position))
            {
                int target_x, target_y;
                projector_target.forward(position.lon, position.lat, target_x, target_y);

                if (!satellite_proj->get_position(img_x + 1, img_y, position2) && !satellite_proj->get_position(img_x, img_y, position3))
                {
                    int target_x2, target_y2;
                    projector_target.forward(position2.lon, position2.lat, target_x2, target_y2);

                    int target_x3, target_y3;
                    projector_target.forward(position3.lon, position3.lat, target_x3, target_y3);

                    int new_radius1 = abs(target_x2 - target_x);
                    int new_radius2 = abs(target_y2 - target_y);

                    int new_radius = std::max(new_radius1, new_radius2);

                    int max = img_pro.images[0].image.width() / 16;

                    if (new_radius < max)
                        radius = new_radius;
                    else if (new_radius1 < max)
                        radius = new_radius1;
                    else if (new_radius2 < max)
                        radius = new_radius2;
                }

                uint16_t color[3];
                color[0] = img_pro.images[3].image.channel(0)[img_y * img_pro.images[0].image.width() + img_x];
                color[1] = img_pro.images[3].image.channel(0)[img_y * img_pro.images[0].image.width() + img_x];
                color[2] = img_pro.images[3].image.channel(0)[img_y * img_pro.images[0].image.width() + img_x];

                final_image.draw_circle(target_x, target_y, radius, color, true);
            }
        }

        logger->info("{:d} / {:d}", img_y, img_pro.images[0].image.height());
    }

#endif

    nlohmann::ordered_json fdsfsdf = img_pro.get_proj_cfg();
    fdsfsdf["yaw_offset"] = 2;
    fdsfsdf["roll_offset"] = -1.5;
    fdsfsdf["scan_angle"] = 110;
    fdsfsdf["timestamp_offset"] = 0;

    std::vector<satdump::projection::GCP> gcps = satdump::gcp_compute::compute_gcps(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_d_mersi2.json")), img_pro.get_tle(), img_pro.get_timestamps(0));

    satdump::ImageCompositeCfg rgb_cfg;
    rgb_cfg.equation = "ch3,ch2,ch1"; //"(ch3 * 0.4 + ch2 * 0.6) * 2.2 - 0.15, ch2 * 2.2 - 0.15, ch1 * 2.2 - 0.15";
    rgb_cfg.equalize = true;

    img_pro.images[0].image.equalize();
    img_pro.images[0].image.to_rgb();

    satdump::warp::WarpOperation operation;
    operation.ground_control_points = gcps;
    operation.input_image = satdump::make_composite_from_product(img_pro, rgb_cfg);
    operation.output_width = 2048 * 32;
    operation.output_height = 1024 * 32;

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
                                   [&projector](float lat, float lon, int map_height2, int map_width2) -> std::pair<int, int>
                                   {
                                       int x, y;
                                       projector.forward(lon, lat, x, y);
                                       return {x, y};
                                   });

    // img_map.crop(p_x_min, p_y_min, p_x_max, p_y_max);
    logger->info("Saving...");

    result.output_image.save_png("test.png");

#if 0
    unsigned short color[3] = {0, 65535, 0};
    map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                   final_image,
                                   color,
                                   [&projector_target](float lat, float lon, int map_height2, int map_width2) -> std::pair<int, int>
                                   {
                                       int x, y;
                                       projector_target.forward(lon, lat, x, y);
                                       return {x, y};
                                   });

    logger->info("Saving...");
    final_image.save_png("test.png");
#endif
}
