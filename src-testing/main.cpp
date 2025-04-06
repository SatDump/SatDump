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
#include "products2/punctiform_product.h"

#include "common/image/io.h"

int main(int argc, char *argv[])
{
    initLogger();

#if 0

#else
    satdump::products::PunctiformProduct p;
    p.load("/tmp/jason3_test/AMR-2_RAD/product.cbor");

    image::Image map;
    image::load_jpeg(map, "resources/maps/nasa.jpg");

    int channel = 1;
    int img_x = 200; // map.width();
    int img_y = 100; // map.height();

    struct Pixel
    {
        int x;
        int y;
        std::vector<double> color;
    };

    std::vector<Pixel> all_pixels;

    for (int i = 0; i < p.data[channel].data.size(); i++)
    {
        auto &v = p.data[0];

        auto satpos = p.get_sample_position(0, i); // tracker.get_sat_position_at(v.timestamps[i]);

        int image_y = img_y - ((90.0f + satpos.lat) / 180.0f) * img_y;
        int image_x = (satpos.lon / 360) * img_x + (img_x / 2);

        image_y = image_y % img_y;
        image_x = image_x % img_x;

        printf("%d %d %f\n", image_x, image_y, v.data[i]);

        // std::vector<double> color = {(p.data[2].data[i].value - 12500) / 1000.0,
        //                              (p.data[1].data[i].value - 12600) / 1000.0,
        //                              (p.data[0].data[i].value - 12500) / 1000.0,
        //                              1};

        // std::vector<double> color = {(p.data[1].data[i].value - 12200) / 1000.0,
        //                              (p.data[1].data[i].value - 12200) / 1000.0,
        //                              (p.data[1].data[i].value - 12200) / 1000.0,
        //                              1};

        // std::vector<double> color = {(p.data[1].data[i].value - 13400) / 1000.0,
        //                              (p.data[1].data[i].value - 13400) / 1000.0,
        //                              (p.data[1].data[i].value - 13400) / 1000.0,
        //                              1};

        std::vector<double> color = {(v.data[i] - 12500) / 1000.0,
                                     (v.data[i] - 12500) / 1000.0,
                                     (v.data[i] - 12500) / 1000.0,
                                     1};

        for (auto &c : color)
        {
            if (c > 1.0)
                c = 1;
            if (c < 0)
                c = 0;
        }

        all_pixels.push_back({image_x, image_y, color});

        map.draw_circle(image_x % img_x, image_y, 2, color, true);
    }

    image::save_jpeg(map, "/home/alan/Downloads/test_jason_new.jpg");
    map.fill(0);

    for (int x = 0; x < img_x; x++)
    {
        logger->info(x);
        for (int y = 0; y < img_y; y++)
        {
            int best_id = 0;
            float best_dist = 1e6;
            for (int p = 0; p < all_pixels.size(); p++)
            {
                float dx = all_pixels[p].x - x;
                float dy = all_pixels[p].y - y;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist < best_dist)
                {
                    best_dist = dist;
                    best_id = p;
                }
            }

            if (best_dist < 5)
                map.draw_pixel(x, y, all_pixels[best_id].color);
        }
    }

    map.crop(0, 0, 200, 100);
    // map.resize_bilinear(2048, 1024);
    map.resize(2048, 1024);

    image::save_jpeg(map, "/home/alan/Downloads/test_jason2_new.jpg");
#endif
}