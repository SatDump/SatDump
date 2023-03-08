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
#include "resources.h"
#include <vector>

int main(int argc, char *argv[])
{
    initLogger();

    image::Image<uint8_t> img = image::Image<uint8_t>(10000, 300, 3);
    uint8_t color[] = {255, 255, 255};

#if 1
    img.init_font(resources::getResourcePath("fonts/ComicSansMS3.ttf"));
    for (int i = 0; i < 100; i ++)
       img.draw_text(0, 0, color, 500, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\nUt enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\nDuis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.\nExcepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
#else
    std::vector<image::Image<uint8_t>> font = image::make_font(500);
    for (int i = 0; i < 100; i ++)
        img.draw_text(0, 0, color, font, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\nUt enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\nDuis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.\nExcepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
#endif   
    img.save_png("test_font.png");
}
