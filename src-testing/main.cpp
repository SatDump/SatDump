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

#include "common/image2/image.h"
#include "common/image2/io.h"

int main(int argc, char *argv[])
{
    initLogger();
    completeLoggerInit();

    image2::Image img;
    image2::load_img(img, argv[1]);
    logger->info("Bit Depth %d, Size %d", img.depth(), img.size() * img.typesize());
    img = img.to16bits();
    logger->info("Bit Depth %d, Size %d", img.depth(), img.size() * img.typesize());
    image2::save_img(img, argv[2]);
}