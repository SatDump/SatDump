
#include "logger.h"
#include "common/map/map_drawer.h"
#include "core/resources.h"
#include "nlohmann/json_utils.h"
#include "core/config.h"
#include "common/projection/reprojector.h"

int main(int argc, char *argv[])
{
    initLogger();
    completeLoggerInit();

    if (argc < 4)
    {
        logger->critical("Usage : ./apply_mapoverlay source.png proj.json output.png");
        return 1;
    }

    image::Image<uint8_t> img;
    img.load_png(argv[1]);
    img.to_rgb();

    nlohmann::json proj_cfg = loadJsonFile(argv[2]);

    unsigned char color[3] = {0, 255, 0};
    map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                   img,
                                   color,
                                   satdump::reprojection::setupProjectionFunction(img.width(), img.height(), proj_cfg, {}));

    // img_map.crop(p_x_min, p_y_min, p_x_max, p_y_max);
    logger->info("Saving...");

    img.save_png(argv[3]);
}
