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

    satdump::warp::WarpResult result1, result2;

    {
        auto json_stuff = loadJsonFile(argv[2]);

        satdump::warp::WarpOperation operation;
        operation.input_image.load_img(argv[1]);
        operation.input_image.equalize();
        operation.output_rgba = true;
        operation.ground_control_points = satdump::gcp_compute::compute_gcps(loadJsonFile(argv[3]),
                                                                             json_stuff["tles"],
                                                                             json_stuff["timestamps"],
                                                                             operation.input_image.width(),
                                                                             operation.input_image.height());
        operation.output_width = 2048 * 4;
        operation.output_height = 1024 * 4;

        satdump::warp::ImageWarper warper;
        warper.op = operation;
        warper.update();

        result1 = warper.warp();
    }

    {
        auto json_stuff = loadJsonFile(argv[2 + 3]);

        satdump::warp::WarpOperation operation;
        operation.input_image.load_img(argv[1 + 3]);
        operation.input_image.equalize();
        operation.output_rgba = true;
        operation.ground_control_points = satdump::gcp_compute::compute_gcps(loadJsonFile(argv[3 + 3]),
                                                                             json_stuff["tles"],
                                                                             json_stuff["timestamps"],
                                                                             operation.input_image.width(),
                                                                             operation.input_image.height());
        operation.output_width = 2048 * 4;
        operation.output_height = 1024 * 4;

        satdump::warp::ImageWarper warper;
        warper.op = operation;
        warper.update();

        result2 = warper.warp();
    }

    int final_size_x = 1024;
    int final_size_y = 512;
    image::Image<uint16_t> final_img;
    geodetic::projection::EquirectangularProjection projector;

    // Reproject both into one...
    {
        geodetic::projection::EquirectangularProjection projector1, projector2;
        projector1.init(result1.output_image.width(), result1.output_image.height(), result1.top_left.lon, result1.top_left.lat, result1.bottom_right.lon, result1.bottom_right.lat);
        projector2.init(result2.output_image.width(), result2.output_image.height(), result2.top_left.lon, result2.top_left.lat, result2.bottom_right.lon, result2.bottom_right.lat);

        projector.init(final_size_x, final_size_y, std::min(result1.top_left.lon, result2.top_left.lon),
                       std::max(result1.top_left.lat, result2.top_left.lat),
                       std::max(result1.bottom_right.lon, result2.bottom_right.lon),
                       std::min(result1.bottom_right.lat, result2.bottom_right.lat));
        final_img.init(final_size_x, final_size_y, 4);

        float lon, lat;
        int x2, y2;
        for (int x = 0; x < (int)final_img.width(); x++)
        {
            for (int y = 0; y < (int)final_img.height(); y++)
            {
                projector.reverse(x, y, lon, lat);
                if (lon == -1 || lat == -1)
                    continue;

                projector1.forward(lon, lat, x2, y2);
                if (x2 != -1 || y2 != -1)
                    for (int c = 0; c < result1.output_image.channels(); c++)
                        if (result1.output_image.channel(3)[y2 * result1.output_image.width() + x2])
                            final_img.channel(c)[y * final_img.width() + x] = result1.output_image.channel(c)[y2 * result1.output_image.width() + x2];

                projector2.forward(lon, lat, x2, y2);
                if (x2 != -1 || y2 != -1)
                    for (int c = 0; c < result2.output_image.channels(); c++)
                        if (result2.output_image.channel(3)[y2 * result2.output_image.width() + x2])
                            final_img.channel(c)[y * final_img.width() + x] = result2.output_image.channel(c)[y2 * result2.output_image.width() + x2];
            }
        }
    }

    unsigned short color[4] = {0, 65535, 0, 65535};
    map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                   final_img,
                                   color,
                                   [&projector](float lat, float lon, int, int) -> std::pair<int, int>
                                   {
                                       int x, y;
                                       projector.forward(lon, lat, x, y);
                                       return {x, y};
                                   });

#if 0
    for (auto gcp : operation.ground_control_points)
    {
        auto projfunc = [&projector](float lat, float lon, int, int) -> std::pair<int, int>
        {
            int x, y;
            projector.forward(lon, lat, x, y);
            return {x, y};
        };

        std::pair<int, int> pos = projfunc(gcp.lat, gcp.lon, 0, 0);

        if (pos.first == -1 || pos.second == -1)
            continue;

        result.output_image.draw_circle(pos.first, pos.second, 2, color, true);
    }
#endif

    // img_map.crop(p_x_min, p_y_min, p_x_max, p_y_max);
    logger->info("Saving...");

    final_img.save_png("test_ascat.png");
}
