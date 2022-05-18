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

int main(int argc, char *argv[])
{
    initLogger();

    std::string user_path = std::string(getenv("HOME")) + "/.config/satdump";
    satdump::config::loadConfig("satdump_cfg.json", user_path);

    // satdump::process_dataset("metop_ahrpt_new/dataset.json");
    // satdump::process_dataset("jpss_bb_test/dataset.json");
    satdump::process_dataset("m22_proj_test1/dataset.json");

    /*
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

    nlohmann::json metop_mhs_cfg;
    // metop_mhs_cfg.tles = img_pro.get_tle();
    // metop_mhs_cfg.timestamps = img_pro.get_timestamps(0);
    // metop_mhs_cfg.image_width = 90;
    // metop_mhs_cfg.timestamp_offset = -2;
    // metop_mhs_cfg.scan_angle = 100;
    // metop_mhs_cfg.roll_offset = -1.1;
    // metop_mhs_cfg.gcp_spacing_x = 5;
    // metop_mhs_cfg.gcp_spacing_y = 5;

    // logger->trace("\n" + img_pro.contents.dump(4));

    std::vector<satdump::projection::GCP> gcps = satdump::gcp_compute::compute_gcps(img_pro.get_proj_cfg(), img_pro.get_tle(), img_pro.get_timestamps());

    satdump::ImageCompositeCfg rgb_cfg;
    rgb_cfg.equation = "ch3,ch2,ch1"; //"(ch3 * 0.4 + ch2 * 0.6) * 2.2 - 0.15, ch2 * 2.2 - 0.15, ch1 * 2.2 - 0.15";
    rgb_cfg.equalize = true;

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
    */
}
