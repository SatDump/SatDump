
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

int main(int /*argc*/, char *argv[])
{
    initLogger();

    std::string user_path = std::string(getenv("HOME")) + "/.config/satdump";
    satdump::config::loadConfig("satdump_cfg.json", user_path);

    satdump::ImageProducts img_pro;
    img_pro.load(argv[1]);

    logger->trace("\n" + img_pro.contents.dump(4));

    std::vector<satdump::projection::GCP> gcps =
        satdump::gcp_compute::compute_gcps(loadJsonFile(argv[2]),
                                           img_pro.get_tle(),
                                           img_pro.get_timestamps(0));
    // filter_timestamps_simple(img_pro.get_timestamps(0), 1e4, 10));

    satdump::ImageCompositeCfg rgb_cfg;
    rgb_cfg.equation = "1-ch10,1-ch10,1-ch10"; //"(ch3 * 0.4 + ch2 * 0.6) * 2.2 - 0.15, ch2 * 2.2 - 0.15, ch1 * 2.2 - 0.15";
    rgb_cfg.equalize = true;
    rgb_cfg.white_balance = true;

    // img_pro.images[0].image.equalize();
    // img_pro.images[0].image.to_rgb();

    satdump::warp::WarpOperation operation;
    operation.ground_control_points = gcps;
    operation.input_image = satdump::make_composite_from_product(img_pro, rgb_cfg);
    operation.output_width = 2048 * 4;
    operation.output_height = 1024 * 4;

    satdump::warp::ImageWarper warper;
    warper.op = operation;
    warper.update();

    satdump::warp::WarpResult result = warper.warp();

    geodetic::projection::EquirectangularProjection projector;
    projector.init(result.output_image.width(), result.output_image.height(), result.top_left.lon, result.top_left.lat, result.bottom_right.lon, result.bottom_right.lat);

    unsigned short color[3] = {0, 65535, 0};
    map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                   result.output_image,
                                   color,
                                   [&projector](float lat, float lon, int, int) -> std::pair<int, int>
                                   {
                                       int x, y;
                                       projector.forward(lon, lat, x, y);
                                       return {x, y};
                                   });

    // img_map.crop(p_x_min, p_y_min, p_x_max, p_y_max);
    logger->info("Saving...");

    result.output_image.save_png("test.png");
}
