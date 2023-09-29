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

#include "common/image/earth_curvature.h"

int main(int argc, char *argv[])
{
    initLogger();

    image::Image<uint16_t> image1;
    image1.load_png(argv[1]);

    auto image2 = image::earth_curvature::correct_earth_curvature(image1, 820, 2800, 1);

    image2.save_png(argv[2]);
}