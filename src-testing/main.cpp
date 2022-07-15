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

#include "common/projection/sat_proj/sat_proj.h"
#include "common/geodetic/vincentys_calculations.h"

int main(int argc, char *argv[])
{
    initLogger();

    std::string user_path = std::string(getenv("HOME")) + "/.config/satdump";
    // satdump::config::loadConfig("satdump_cfg.json", user_path);

    satdump::ImageProducts img_pro;
    img_pro.load(argv[1]);

    satdump::SatelliteTracker sat_tracker(img_pro.get_tle());
    std::shared_ptr<satdump::SatelliteProjection> sat_projection = get_sat_proj(img_pro.get_proj_cfg(), img_pro.get_tle(), img_pro.get_timestamps());

    geodetic::geodetic_coords_t pos1, pos2, sat_pos;

    sat_projection->get_position(0, 0, pos1);
    sat_projection->get_position(img_pro.get_proj_cfg()["image_width"].get<int>() - 1, 0, pos2);
    sat_pos = sat_tracker.get_sat_position_at(img_pro.get_timestamps()[0]);

    geodetic::geodetic_curve_t res = geodetic::vincentys_inverse(pos1, pos2);

    //  logger->info("Swath is around {:.0f} Km, Altitude is {:.0f} Km", res.distance, );
}