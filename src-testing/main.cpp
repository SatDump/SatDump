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

#include "common/dsp/utils/random.h"
#include <fstream>

#include "products/image_products.h"

#include "nlohmann/json_utils.h"

int main(int argc, char *argv[])
{
    initLogger();

#if 0
    std::ifstream data_in(argv[1], std::ios::binary);
    std::ofstream data_out(argv[2], std::ios::binary);

    uint8_t buffer1[1024];
    int8_t buffer2[8192];

    dsp::Random gaussian;

    while (!data_in.eof()) //&& lines_img < 500)
    {
        data_in.read((char *)buffer1, 1024);

        int bit2pos = 0;
        for (int i = 0; i < 1024; i++)
            for (int x = 6; x >= 0; x -= 2)
            {
                buffer2[bit2pos++] = (((buffer1[i] >> (x + 0)) & 1) ? 70 : -70) + gaussian.gasdev() * 10;
                buffer2[bit2pos++] = (((buffer1[i] >> (x + 1)) & 1) ? 70 : -70) + gaussian.gasdev() * 10;
            }

        data_out.write((char *)buffer2, bit2pos);
    }
#endif

    satdump::ImageProducts msu_gs_products;
    msu_gs_products.instrument_name = "msu_gs";
    msu_gs_products.has_timestamps = false;
    msu_gs_products.needs_correlation = false;
    // msu_gs_products.set_tle(satellite_tle);
    msu_gs_products.bit_depth = 10;
    //  msu_gs_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_MULTIPLE_LINES;
    msu_gs_products.set_proj_cfg(loadJsonFile("/home/alan/Downloads/elektrol2.json"));

    image::Image<uint16_t> img;

    for (int i = 0; i < 10; i++)
    {
        img.load_img("/home/alan/Downloads/230929_1530/230929_1530_" + std::to_string(i + 1) + ".jpg");
        msu_gs_products.images.push_back({"MSU-GS-" + std::to_string(i + 1), std::to_string(i + 1), img});
    }

    msu_gs_products.save("./msu_gs_products_l2_test");
}