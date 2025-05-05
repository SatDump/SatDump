
#include "logger.h"
#include "products/image_products.h"
#include "common/map/map_drawer.h"
#include "resources.h"
#include "nlohmann/json_utils.h"
#include "core/config.h"
#include "common/projection/warp/warp.h"
#include "common/projection/gcp_compute/gcp_compute.h"
#include "common/projection/projs/equirectangular.h"
#include "common/utils.h"
#include "init.h"
#include "common/geodetic/vincentys_calculations.h"
#include "common/image/io.h"

#include "common/projection/warp/warp_bkd.h"

geodetic::geodetic_coords_t calculate_center_of_points(std::vector<geodetic::geodetic_coords_t> points)
{
    double x_total = 0;
    double y_total = 0;
    double z_total = 0;

    for (auto &pt : points)
    {
        pt.toRads();
        x_total += cos(pt.lat) * cos(pt.lon);
        y_total += cos(pt.lat) * sin(pt.lon);
        z_total += sin(pt.lat);
    }

    x_total /= points.size();
    y_total /= points.size();
    z_total /= points.size();

    double lon = atan2(y_total, x_total);
    double hyp = sqrt(x_total * x_total + y_total * y_total);
    double lat = atan2(z_total, hyp);

    return geodetic::geodetic_coords_t(lat, lon, 0, true).toDegs();
}

int main(int /*argc*/, char *argv[])
{
    initLogger();

    // std::string user_path = std::string(getenv("HOME")) + "/.config/satdump";
    // satdump::config::loadConfig("satdump_cfg.json", user_path);
    // initFileSink();

    // We don't wanna spam with init this time around
    logger->set_level(slog::LOG_OFF);
    satdump::initSatdump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    satdump::ImageProducts img_pro;
    img_pro.load(argv[1]);

    printf("\n%s\n", img_pro.contents.dump(4).c_str());

    satdump::ImageCompositeCfg rgb_cfg;
    rgb_cfg.equation = "ch3,ch2,ch1"; //"(ch7421+ch7422+ch7423+ch7242)/4";
    //    rgb_cfg.equation = "1-ch37";
    // rgb_cfg.equation = "1-ch33,1-ch34,1-ch35"; //"(ch3 * 0.4 + ch2 * 0.6) * 2.2 - 0.15, ch2 * 2.2 - 0.15, ch1 * 2.2 - 0.15";
    rgb_cfg.equalize = true;
    // rgb_cfg.white_balance = true;
    // rgb_cfg.normalize = true;
    // rgb_cfg.apply_lut = true;

    // img_pro.images[0].image.equalize();
    // img_pro.images[0].image.to_rgb();

    // filter_timestamps_simple(img_pro.get_timestamps(0), 1e4, 10));

    satdump::warp::WarpOperation operation_t;
    std::vector<double> final_tt;
    nlohmann::json final_mtd;
    image::Image input_image = satdump::make_composite_from_product(img_pro, rgb_cfg, nullptr, &final_tt, &final_mtd);
    input_image = input_image.to16bits();
    operation_t.input_image = &input_image;
    // operation_t.input_image.median_blur();
    nlohmann::json proj_cfg = img_pro.get_proj_cfg(); // loadJsonFile(argv[2]);
    proj_cfg["metadata"] = final_mtd;
    proj_cfg["metadata"]["tle"] = img_pro.get_tle();
    proj_cfg["metadata"]["timestamps"] = final_tt;
    printf("%s\n", final_mtd.dump(4).c_str());
    operation_t.ground_control_points = satdump::gcp_compute::compute_gcps(proj_cfg,
                                                                           operation_t.input_image->width(),
                                                                           operation_t.input_image->height());
    operation_t.output_width = 2048 * 2;  // 10;
    operation_t.output_height = 1024 * 2; // 10;
    operation_t.output_rgba = true;

#if 0
    auto warp_result = satdump::warp::performSmartWarp(operation_t);
#else
    satdump::warp::ImageWarper warper;
    warper.op = operation_t;
    warper.update();
    auto warp_result = warper.warp();
#endif

    logger->info("Wrapped!");
    geodetic::projection::EquirectangularProjection projector_final;
    projector_final.init(warp_result.output_image.width(), warp_result.output_image.height(), warp_result.top_left.lon, warp_result.top_left.lat, warp_result.bottom_right.lon, warp_result.bottom_right.lat);

    logger->info("Map overlay!");
    std::vector<double> color = {0, 1, 0, 1};
    map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                   warp_result.output_image,
                                   color,
                                   [&projector_final](float lat, float lon, int, int) -> std::pair<int, int>
                                   {
                                       int x, y;
                                       projector_final.forward(lon, lat, x, y);
                                       return {x, y};
                                   });

    {
        std::vector<double> color = {1, 1, 0, 1};
        map::drawProjectedMapShapefile(
            {resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
            warp_result.output_image,
            color,
            [&projector_final](float lat, float lon, int, int) -> std::pair<int, int>
            {
                int x, y;
                projector_final.forward(lon, lat, x, y);
                return {x, y};
            });
    }
#if 1
    {

        std::vector<double> color = {0, 0, 1, 1};

        map::drawProjectedMapLatLonGrid(warp_result.output_image,
                                        color,
                                        [&projector_final](float lat, float lon, int, int) -> std::pair<int, int>
                                        {
                                            int x, y;
                                            projector_final.forward(lon, lat, x, y);
                                            return {x, y};
                                        });
    }
#endif

    image::save_img(warp_result.output_image, "test");
}
