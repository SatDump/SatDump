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
#include <iostream>
#include "common/image/image.h"

int main(int argc, char *argv[])
{
    initLogger();

    std::ifstream input_bin(argv[1], std::ios::binary);

    std::vector<uint8_t> full_file;

    {
        uint8_t v;
        while (!input_bin.eof())
        {
            input_bin.read((char *)&v, 1);
            full_file.push_back(v);
        }
    }

    std::vector<uint16_t> images[5];

    for (size_t i = 0; i < full_file.size(); i++)
    {
        if (full_file[i + 0] == 0xa2 &&
            full_file[i + 1] == 0xf0 &&
            full_file[i + 2] == 0xff &&
            full_file[i + 3] == 0xde)
        {
            // printf("HEY\n");

            for (int c = 0; c < 409; c++)
            {
                for (int v = 0; v < 5; v++)
                {
                    uint16_t vv = full_file[i + 1214 + (c * 5 + v) * 2 + 0] << 8 | full_file[i + 1214 + (c * 5 + v) * 2 + 1];
                    images[v].push_back(vv);
                }
            }
        }
    }

    for (int v = 0; v < 5; v++)
    {
        image::Image<uint16_t> img(images[v].data(), 409, images[v].size() / 409, 1);
        img.equalize();
        img.save_png("noaa_test_" + std::to_string(v + 1) + ".png");
    }
}