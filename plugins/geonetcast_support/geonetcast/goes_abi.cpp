#include "goes_abi.h"
#include "libs/miniz/miniz.h"

namespace geonetcast
{
    image::Image<uint16_t> parse_goesr_abi_netcdf_fulldisk(std::vector<uint8_t> data, int side_chunks, int bit_depth, bool mode2)
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
                current_buf_pos += input_size;
                // logger->info("%d %d, %d", input_size, output_size, val);

                for (int i = 0; i < chunk_width * chunk_width; i++)
                    chunk_image[i] = (decompressed_chunk[chunk_width * chunk_width + i] << 8 | decompressed_chunk[i]) << bit_shift;

                if (img_cnt_pos < side_chunks * side_chunks)
                    final_image.draw_image(0, chunk_image, (img_cnt_pos % side_chunks) * chunk_width, (img_cnt_pos / side_chunks) * chunk_width);

                img_cnt_pos++;

                // chunk_image.save_png("ABI/TEST_FULL_ABI" + std::to_string(img_cnt_pos) + ".png");
            }
            else
            {
                current_buf_pos++;
            }
        }

        delete[] decompressed_chunk;

        return final_image;
    }
}