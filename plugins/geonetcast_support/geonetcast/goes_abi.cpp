#include "goes_abi.h"
#include "libs/miniz/miniz.h"

#ifdef ENABLE_HDF5_PARSING
#include <hdf5.h>
#include <H5LTpublic.h>
#endif

namespace geonetcast
{
#ifdef ENABLE_HDF5_PARSING
    image::Image parse_goesr_abi_netcdf_fulldisk_CMI(std::vector<uint8_t> data, int bit_depth)
    {
        // herr_t status;
        hsize_t image_dims[2];

        hid_t file = H5LTopen_file_image(data.data(), data.size(), H5F_ACC_RDONLY);

        if (file < 0)
            return image::Image<uint16_t>();

        hid_t dataset = H5Dopen2(file, "CMI", H5P_DEFAULT);

        if (dataset < 0)
            return image::Image<uint16_t>();

        hid_t dataspace = H5Dget_space(dataset); /* dataspace handle */
        int rank = H5Sget_simple_extent_ndims(dataspace);
        /*int status_n =*/H5Sget_simple_extent_dims(dataspace, image_dims, NULL);

        if (rank != 2)
            return image::Image<uint16_t>();

        hid_t memspace = H5Screate_simple(2, image_dims, NULL);

        image::Image image_out(16, image_dims[0], image_dims[1], 1);

        /*status =*/H5Dread(dataset, H5T_NATIVE_UINT16, memspace, dataspace, H5P_DEFAULT, (uint16_t *)image_out.raw_data());

        for (size_t i = 0; i < image_out.size(); i++)
            image_out.set(i image_out.get(i) << (16 - bit_depth));

        H5Dclose(dataset);
        H5Fclose(file);

        return image_out;
    }
#endif

#if 0
    image::Image parse_goesr_abi_netcdf_fulldisk(std::vector<uint8_t> data, int side_chunks, int bit_depth, bool mode2)
    {
        const int chunk_width = mode2 ? 226 : 1356;

        image::Image chunk_image(16, chunk_width, chunk_width, 1);
        image::Image final_image(16, chunk_width * side_chunks, chunk_width * side_chunks, 1);
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

                // chunk_image.save_img("ABI/TEST_FULL_ABI" + std::to_string(img_cnt_pos));
            }
            else
            {
                current_buf_pos++;
            }
        }

        delete[] decompressed_chunk;

        return final_image;
    }
#endif
}