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

    image::Image<uint8_t> image_1, image_2, image_3, image_rgb;

    image_1.load_png(argv[1]);
    image_2.load_png(argv[2]);
    image_3.load_png(argv[3]);

    logger->info("Processing");
    image_rgb.init(image_1.width(), image_1.height(), 3);

    image_rgb.draw_image(0, image_3, -30, 12);
    image_rgb.draw_image(1, image_2, 0);
    image_rgb.draw_image(2, image_1, 23, -17);

    logger->info("Saving");
    image_rgb.save_png("msu_compo.png");
}