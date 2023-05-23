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

std::ofstream test_idk("test_hmmmm.bin");

image::Image<uint16_t> parse_goesr_abi_netcdf_fulldisk(std::vector<uint8_t> &data, int side_chunks, int bit_depth, bool mode2 = false)
{
    const int chunk_width = mode2 ? 226 : 1356;

    image::Image<uint16_t> chunk_image(chunk_width, chunk_width, 1);
    image::Image<uint16_t> final_image(chunk_width * side_chunks, chunk_width * side_chunks, 1);
    uint8_t *decompressed_chunk = new uint8_t[chunk_width * chunk_width * 2];

    int mz_out = MZ_OK;
    int current_buf_pos = 0;
    int img_cnt_pos = 0;
    int bit_shift = 16 - bit_depth;

    while (current_buf_pos < data.size() - 4)
    {
        if (data[current_buf_pos] != 0x78)
        {
            current_buf_pos++;
            continue;
        }

        mz_ulong output_size = chunk_width * chunk_width * 2;
        mz_ulong input_size = data.size();
        mz_out = mz_uncompress2(decompressed_chunk, &output_size, &data[current_buf_pos], &input_size);

        // logger->critical(mz_out);

        if (mz_out == MZ_OK && output_size == (mode2 ? 102152 : 3677472))
        {
            int val = data[current_buf_pos - 6] << 8 | data[current_buf_pos - 5];

            test_idk.write((char *)&data[current_buf_pos - 8], 8);

            current_buf_pos += input_size;
            logger->info("%d %d, %d", input_size, output_size, val);

            for (int i = 0; i < chunk_width * chunk_width; i++)
                chunk_image[i] = (decompressed_chunk[chunk_width * chunk_width + i] << 8 | decompressed_chunk[i]) << bit_shift;

            if (img_cnt_pos < side_chunks * side_chunks)
                final_image.draw_image(0, chunk_image, (img_cnt_pos % side_chunks) * chunk_width, (img_cnt_pos / side_chunks) * chunk_width);

            img_cnt_pos++;

            chunk_image.save_png("ABI/TEST_FULL_ABI" + std::to_string(img_cnt_pos) + ".png");
        }
        else
        {
            current_buf_pos++;
        }
    }

    delete[] decompressed_chunk;

    return final_image;
}

#include <filesystem>

int main(int argc, char *argv[])
{
    initLogger();

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

    int mode = 0;
    int channel = 0;
    int satellite = 0;
    uint64_t tstart = 0;
    uint64_t tend = 0;
    uint64_t tfile = 0;
    uint64_t tidk = 0;

    std::string filepath(argv[1]);
    std::string filename = std::filesystem::path(filepath).stem().string();

    if (sscanf(filename.c_str(),
               "OR_ABI-L2-CMIPF-M%1dC%2d_G%2d_s%lu_e%lu_c%lu-%lu_0.nc",
               &mode, &channel, &satellite,
               &tstart, &tend, &tfile, &tidk) == 7)
    {
        logger->info("Mode %d Channel %d Satellite %d", mode, channel, satellite);

        int side_chunk_size = 4;
        int bit_depth = 12;

        if (channel == 1 || channel == 2 || channel == 3 || channel == 5)
            side_chunk_size = 8;

        if (channel == 7)
            bit_depth = 14;

        image::Image<uint16_t> final_image = parse_goesr_abi_netcdf_fulldisk(data_vector, side_chunk_size, bit_depth);

        logger->info("DONE");

        final_image.save_jpeg("GOES_FINAL.jpg");
    }
    else if (sscanf(filename.c_str(),
                    "OR_ABI-L2-CMIPF-M%1dC%2d_G%2d_s%lu_e%lu_c%lu.nc",
                    &mode, &channel, &satellite,
                    &tstart, &tend, &tfile) == 6)
    {
       
    }
}
