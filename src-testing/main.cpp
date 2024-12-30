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
#include "products2/image_product.h"
#include "common/image/io.h"
#include "common/image/processing.h"

#include "products2/image/product_equation.h"

#include "common/utils.h"

int main(int argc, char *argv[])
{
    initLogger();

    satdump::products::ImageProduct products;
    products.load(argv[1]);

#if 0
    image::Image ch1, ch9;
    ch1 = products.get_channel_image("19").image;
    ch9 = products.get_channel_image("10").image;

    image::Image rgbimg(ch1.depth(), ch1.width(), ch1.height(), 3);

    satdump::ChannelTransform ch1t = products.get_channel_image("19").ch_transform;
    satdump::ChannelTransform ch9t = products.get_channel_image("10").ch_transform;

    // ch1t.init_affine(0.95, 1, 0, -1);
    // ch1t.init_affine_slantx(0.93, 1, 2.8, 4,
    //                         145.0 / 2.0,
    //                         6.0 / (145.0 / 2.0));

    printf("TYPE IS %f\n", ch1t.getType());

    image::equalize(ch1, true);
    image::equalize(ch9, true);

    for (double x = 0; x < rgbimg.width(); x++)
    {
        for (double y = 0; y < rgbimg.height(); y++)
        {
            double ch1_x = x, ch1_y = y;
            double ch9_x = x, ch9_y = y;

            ch1t.forward(&ch9_x, &ch9_y);
            ch9t.reverse(&ch9_x, &ch9_y);
            // ch9_y += ((ch9_x - (145.0 / 2.0)) / (145.0 / 2.0)) * 11;

            //  printf("%f %f - %f %f\n", ch1_x, ch1_y, ch9_x, ch9_y);

            rgbimg.set(0, x, y, ch1.get_pixel_bilinear(0, ch1_x, ch1_y));
            rgbimg.set(1, x, y, ch1.get_pixel_bilinear(0, ch1_x, ch1_y));
            if (ch9_x > 0 && ch9_x < ch9.width() && ch9_y > 0 && ch9_y < ch9.height())
                rgbimg.set(2, x, y, ch9.get_pixel_bilinear(0, ch9_x, ch9_y));
        }
    }

    // image::equalize(rgbimg, true);
#endif

    double start_time = getTime();
    auto rgbimg = satdump::products::generate_equation_product_composite(&products, "ch3, ch2, ch1");
    double end_time = getTime();

    printf("Time %f\n", end_time - start_time);

    // image::equalize(rgbimg, true);

    image::save_img(rgbimg, argv[2]);
}
