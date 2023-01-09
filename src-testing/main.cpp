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
#include "common/projection/reprojector.h"
#include "common/projection/gcp_compute/gcp_compute.h"
#include "common/projection/projs/equirectangular.h"
#include "common/utils.h"
#include "common/projection/projs/equirectangular.h"
#include "common/projection/sat_proj/sat_proj.h"

int main(int /*argc*/, char *argv[])
{
    initLogger();

    std::string user_path = std::string(getenv("HOME")) + "/.config/satdump";
    satdump::config::loadConfig("satdump_cfg.json", user_path);

    satdump::ImageProducts img_pro;
    img_pro.load(argv[1]);

    satdump::ImageCompositeCfg rgb_cfg;
    // rgb_cfg.equation = "(chb - 0.6)*4";
    // rgb_cfg.equation = "(chb - 0.65)*5";
    rgb_cfg.lua = "scripted_compos/underlay_with_clouds.lua";
    rgb_cfg.channels = "chb";
    rgb_cfg.lua_vars["minoffset"] = 0.32;
    rgb_cfg.lua_vars["scalar"] = 4.0;
    rgb_cfg.lua_vars["thresold"] = 0.00;

    auto input_image = satdump::make_composite_from_product(img_pro, rgb_cfg, nullptr);

#if 0
    image::Image<uint16_t> map_image;
    map_image.load_img("/home/alan/Downloads/world.200408.3x21600x10800 (1).png");

    auto proj_func = satdump::get_sat_proj(img_pro.get_proj_cfg(), img_pro.get_tle(), img_pro.get_timestamps());

    geodetic::projection::EquirectangularProjection equ_proj;
    equ_proj.init(map_image.width(), map_image.height(), -180, 90, 180, -90);

    input_image.to_rgb();

    for (int x = 0; x < input_image.width(); x++)
    {
        for (int y = 0; y < input_image.height(); y++)
        {
            geodetic::geodetic_coords_t pos;
            if (proj_func->get_position(x, y, pos))
                continue;
            int x2, y2;
            equ_proj.forward(pos.lon, pos.lat, x2, y2);

            int val = input_image[y * input_image.width() + x];
            if (val < 10)
                for (int c = 0; c < 3; c++)
                    input_image.channel(c)[y * input_image.width() + x] = map_image.channel(c)[y2 * map_image.width() + x2];
            else
                for (int c = 0; c < 3; c++)
                    input_image.channel(c)[y * input_image.width() + x] = (map_image.channel(c)[y2 * map_image.width() + x2] * 0.4 + val * 0.6);
        }
    }
#endif

    input_image.save_png("test2.png");
}
