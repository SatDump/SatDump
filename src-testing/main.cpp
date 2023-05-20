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

#include <fstream>
#include <vector>
#include "libs/miniz/miniz.h"
#include "common/image/image.h"

int main(int argc, char *argv[])
{
    initLogger();

#if 1
    std::vector<uint8_t> data_vector;

    {
        std::ifstream data_in(argv[1]);
        while (!data_in.eof())
        {
            uint8_t v;
            data_in.read((char *)&v, 1);
            data_vector.push_back(v);
        }
    }

    int mz_out = MZ_OK;
    int current_in_pos = 0;

    int img_cnt_pos = 0;

    int img_rec_width = 1356;
    int img_cnt_height = 4;
    int img_cnt_width = 7;

    image::Image<uint16_t> final_image(img_rec_width * img_cnt_width, img_rec_width * img_cnt_height, 1);

    std::vector<uint8_t> data_buffer_out(img_rec_width * img_rec_width * 2);

    // bool had_one_ok = false;
    while (/*mz_out == MZ_OK &&*/ current_in_pos < data_vector.size())
    {
        if (data_vector[current_in_pos] != 0x78)
        {
            current_in_pos++;
            continue;
        }

        mz_ulong output_size = data_buffer_out.size();
        mz_ulong input_size = data_vector.size();
        mz_out = mz_uncompress2(data_buffer_out.data(), &output_size, &data_vector[current_in_pos], &input_size);

        // logger->critical(mz_out);

        if (mz_out == MZ_OK)
        {
            logger->info(input_size);
            current_in_pos += input_size;

            image::Image<uint16_t> chunk_image(img_rec_width, img_rec_width, 1);

            for (int i = 0; i < img_rec_width * img_rec_width; i++)
                chunk_image[i] = (data_buffer_out[img_rec_width * img_rec_width + i] << 8 | data_buffer_out[i]) << 4;

            int img_pos = 0;

            if (img_cnt_width == 4 && img_cnt_height == 4 && img_rec_width == 1536)
            {

                if (img_cnt_pos < 16)
                    img_pos = img_cnt_pos;
                else if (img_cnt_pos == 30)
                    img_pos = 15;
            }
            else
            {
                img_pos = img_cnt_pos;
            }

            final_image.draw_image(0, chunk_image, (img_pos % img_cnt_width) * img_rec_width, (img_pos / img_cnt_width) * img_rec_width);

            img_cnt_pos++;

            chunk_image.save_png("TEST_FULL_ABI" + std::to_string(img_cnt_pos) + ".png");

            // had_one_ok = true;
        }
        else
        {
            current_in_pos++;
            // if (!had_one_ok)
            //     mz_out = MZ_OK;
        }
    }

    final_image.save_png("GOES_FINAL.png");
#endif
}
