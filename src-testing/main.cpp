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

#include "init.h"
#include "logger.h"
#include "common/image/image.h"
int main(int argc, char *argv[])
{
    initLogger();

    image::Image<uint8_t> lut = image::generate_lut<uint8_t>(1024, {{0.1, {0.0, 0.0, 0.0}},
                                                                   {0.23, {0.23, 0.75, 0.89}},
                                                                   {0.50, {0.1, 0.9, 0.5}},
                                                                   {0.75, {1.0, 0.0, 0.0}},
                                                                   {0.75, {0.0, 1.0, 0.0}},
                                                                   {0.84, {1, 1, 1}},
                                                                   {0.95, {0,0,0}},
                                                                   {1.0, {1.0, 0.0, 0.0}}});
    image::Image<uint8_t> lut2(1024, 100, 3);

    for (int y = 0; y < 100; y++)
    {
        lut2.draw_image(0, image::Image<uint8_t>(lut.channel(0), 1024, 1, 1), 0, y);
        lut2.draw_image(1, image::Image<uint8_t>(lut.channel(1), 1024, 1, 1), 0, y);
        lut2.draw_image(2, image::Image<uint8_t>(lut.channel(2), 1024, 1, 1), 0, y);
    }
    lut2.save_png("lut.png");
}