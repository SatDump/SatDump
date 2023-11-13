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

int main(int /*argc*/, char *argv[])
{
    initLogger();

    image::Image<uint16_t> img_ch1;
    image::Image<uint16_t> img_ch2;

    img_ch1.load_img(argv[1]);
    img_ch2.load_img(argv[2]);
    int offset = std::stoi(argv[4]);

    img_ch1.equalize();
    img_ch2.equalize();

    image::Image<uint16_t> output_rgb(img_ch1.width(), img_ch2.height(), 3);

    output_rgb.draw_image(2, img_ch1);
    output_rgb.draw_image(1, img_ch1);
    output_rgb.draw_image(0, img_ch2, offset);

    output_rgb.save_img(argv[3]);
}