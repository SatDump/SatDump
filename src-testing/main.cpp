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

int main(int argc, char *argv[])
{
    initLogger();

    image::Image<uint16_t> img1, img2;
    img1.load_png("proba1_test/CHRIS/CHRIS-0.png");
    img2.load_png("proba1_test/CHRIS/CHRIS-1.png");

    image::Image<uint16_t> img(img1.width() * 2, img2.height(), 1);

    for (int i = 0; i < img1.height(); i++)
    {
        for (int y = 0; y < img1.width(); y++)
        {
            img[i * img.width() + y * 2 + 0] = img1[i * img1.width() + y];
            img[i * img.width() + y * 2 + 1] = img2[i * img2.width() + y];
        }
    }

    img.save_png("FInal.png");
}