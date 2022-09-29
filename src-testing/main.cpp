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
#include "common/projection/thinplatespline.h"
#include "common/image/histogram_utils.h"

#if 0
image::Image<uint8_t> make_hist_img(std::vector<int> hist)
{
    image::Image<uint8_t> hist_img(hist.size(), 4096, 3);
    uint8_t color[] = {255, 255, 255};
    for (int i = 0; i < hist.size(); i++)
    {
        hist_img.draw_line(i, 0, i, hist[i], color);
    }
    // std::vector<int> all_values;
    // for (int i = 0;)

    hist_img.mirror(false, true);
    // hist_img.resize_bilinear(1000, 500);
    return hist_img;
}
#endif

int main(int /*argc*/, char *argv[])
{
    initLogger();
}
