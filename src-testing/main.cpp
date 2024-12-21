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
#include "common/image/processing.h"
#include "common/image/io.h"

#include "common/projection/thinplatespline.h"

int main(int argc, char *argv[])
{
    initLogger();

    image::Image img1, img2, img3, imgf;

    logger->info("Loading 3");
    image::load_img(img3, argv[1]);
    logger->info("Loading 2");
    image::load_img(img2, argv[2]);
    logger->info("Loading 1");
    image::load_img(img1, argv[3]);

    logger->info("Process");
    imgf.init(img1.depth(), img1.width(), img2.height(), 3);

    satdump::projection::VizGeorefSpline2D spline_roll_pitch_ch2 = satdump::projection::VizGeorefSpline2D(2);
    satdump::projection::VizGeorefSpline2D spline_roll_pitch_ch3 = satdump::projection::VizGeorefSpline2D(2);

    struct testp
    {
        double x;
        double shift_x;
        double shift_y;
    };

    std::vector<testp> points_ch2 = {
        {72, -3, 1},
        {7981, -2, -4},
    };

    std::vector<testp> points_ch3 = {
        {26, 2, -13},
        {7849, -1, 6}, //-12},
    };

    for (auto item : points_ch2)
    {
        double rp_v[2] = {item.shift_x, item.shift_y};
        spline_roll_pitch_ch2.add_point(item.x, item.x, rp_v);
    }

    for (auto item : points_ch3)
    {
        double rp_v[2] = {item.shift_x, item.shift_y};
        spline_roll_pitch_ch3.add_point(item.x, item.x, rp_v);
    }

    spline_roll_pitch_ch2.solve();
    spline_roll_pitch_ch3.solve();

    double pvars[2];

    for (size_t x = 0; x < img1.width(); x++)
    {
        for (size_t y = 0; y < img1.height(); y++)
        {
            size_t x2_2, y2_2;
            spline_roll_pitch_ch2.get_point(x, x, pvars);
            x2_2 = x - pvars[0];
            y2_2 = y - pvars[1];

            if (x2_2 < 0 || y2_2 < 0 || x2_2 >= img1.width() || y2_2 >= img1.height())
                continue;

            size_t x2_3, y2_3;
            spline_roll_pitch_ch3.get_point(x, x, pvars);
            x2_3 = x - pvars[0];
            y2_3 = y - pvars[1];

            if (x2_3 < 0 || y2_3 < 0 || x2_3 >= img1.width() || y2_3 >= img1.height())
                continue;

            imgf.set(0, x, y, img2.get(0, x2_2, y2_2)); // img3.get(0, x2_3, y2_3));
            imgf.set(1, x, y, img3.get(0, x2_3, y2_3)); // img2.get(0, x2_2, y2_2));
            imgf.set(2, x, y, img1.get(0, x, y));
        }
    }

    logger->info("Saving");
    image::save_img(imgf, argv[4]);
}
