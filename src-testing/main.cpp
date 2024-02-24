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
#include "common/projection/projs2/proj.h"

#include "common/image/image.h"

#include "common/map/map_drawer.h"

#include "common/image/image_meta.h"
#include "common/projection/projs2/proj_json.h"

int main(int argc, char *argv[])
{
    initLogger();
    completeLoggerInit();

#if 0
    proj::projection_t p;

    p.type = proj::ProjType_Geos;
    //   p.proj_offset_x = 418962.397137703;
    //   p.proj_offset_y = -101148.834767705;
    //   p.proj_scalar_x = 60.5849500687633;  // 1;
    //   p.proj_scalar_y = -60.5849500687633; // 1;
    //   p.lam0 = 2 * DEG2RAD;
    //   p.phi0 = 48 * DEG2RAD;
    p.params.altitude = 300000;

    proj::projection_setup(&p);

    double x = 10000; // 15101; // 678108.04;
    double y = 10000; // 16495; // 5496954.89;
    double lon, lat;
    proj::projection_perform_inv(&p, x, y, &lon, &lat);

    logger->info("X %f - Y %f", x, y);
    logger->info("Lon %f - Lat %f", lon, lat);

    proj::projection_perform_fwd(&p, lon, lat, &x, &y);

    logger->info("X %f - Y %f", x, y);
#else

    image::Image<uint16_t> image_test;
    image_test.load_tiff(argv[1]);
    image_test.equalize();
    image_test.normalize();
    // image_test.mirror(false, true);

    logger->info("PROC DONE");

    if (!image::has_metadata(image_test))
    {
        logger->error("Meta error!");
        return 0;
    }

    auto jsonp = image::get_metadata(image_test);
    logger->debug("\n%s", jsonp.dump(4).c_str());
    proj::projection_t p = jsonp["proj_cfg"];
    bool v = proj::projection_setup(&p);

    if (v)
    {
        logger->error("Proj error!");
        return 0;
    }

    {
        double x = 0; // 15101; // 678108.04;
        double y = 0; // 16495; // 5496954.89;
        double lon, lat;
        proj::projection_perform_inv(&p, x, y, &lon, &lat);

        logger->info("X %f - Y %f", x, y);
        logger->info("Lon %f - Lat %f", lon, lat);

        proj::projection_perform_fwd(&p, lon, lat, &x, &y);

        logger->info("X %f - Y %f", x, y);
    }

    {
        unsigned short color[4] = {0, 65535, 0, 65535};
        map::drawProjectedMapShapefile({"resources/maps/ne_10m_admin_0_countries.shp"},
                                       image_test,
                                       color,
                                       [&p](double lat, double lon, int, int) -> std::pair<int, int>
                                       {
                                           double x, y;
                                           proj::projection_perform_fwd(&p, lon, lat, &x, &y);
                                           return {(int)x, (int)y};
                                       });
    }

    image_test.save_tiff(argv[2]);

    proj::projection_free(&p);
#endif
}