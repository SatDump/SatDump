#include "image.h"
#include <jpeglib.h>
#include <stdexcept>

namespace image
{
    int percentile(unsigned short *array, int size, float percentile)
    {
        float number_percent = (size + 1) * percentile / 100.0f;
        if (number_percent == 1)
            return array[0];
        else if (number_percent == size)
            return array[size - 1];
        else
            return array[(int)number_percent - 1] + (number_percent - (int)number_percent) * (array[(int)number_percent] - array[(int)number_percent - 1]);
    }

    void white_balance(cimg_library::CImg<unsigned short> &image, float percentileValue, int channelCount)
    {
        int height = image.height();
        int width = image.width();

        unsigned short *sorted_array = new unsigned short[height * width];

        for (int band_number = 0; band_number < channelCount; band_number++)
        {
            // Load the whole image band into our array
            std::memcpy(sorted_array, &image.data()[band_number * width * height], width * height * sizeof(unsigned short));

            // Sort it
            std::sort(&sorted_array[0], &sorted_array[width * height]);

            // Get percentiles
            int percentile1 = percentile(sorted_array, width * height, percentileValue);
            int percentile2 = percentile(sorted_array, width * height, 100.0f - percentileValue);

            for (int i = 0; i < width * height; i++)
            {
                long balanced = (image[band_number * width * height + i] - percentile1) * 65535.0f / (percentile2 - percentile1);
                if (balanced < 0)
                    balanced = 0;
                else if (balanced > 65535)
                    balanced = 65535;
                image[band_number * width * height + i] = balanced;
            }
        }
    }

    struct jpeg_error_struct
    {
        struct jpeg_error_mgr pub;
        jmp_buf setjmp_buffer;
    };

    static void libjpeg_error_func(j_common_ptr cinfo)
    {
        longjmp(((jpeg_error_struct *)cinfo->err)->setjmp_buffer, 1);
    }

    cimg_library::CImg<unsigned short> decompress_jpeg(uint8_t *data, int length)
    {
        cimg_library::CImg<unsigned short> img;
        unsigned char *jpeg_decomp = NULL;

        // Huge thanks to https://gist.github.com/PhirePhly/3080633
        jpeg_error_struct jerr;
        jpeg_decompress_struct cinfo;

        // Init
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = libjpeg_error_func;

        if (setjmp(jerr.setjmp_buffer))
        {
            // Free memory
            delete[] jpeg_decomp;
            return img;
        }

        jpeg_create_decompress(&cinfo);

        // Parse and start decompressing
        jpeg_mem_src(&cinfo, data, length);
        jpeg_read_header(&cinfo, FALSE);
        jpeg_start_decompress(&cinfo);

        // Init output buffer
        jpeg_decomp = new unsigned char[cinfo.image_width * cinfo.image_height];

        // Decompress
        while (cinfo.output_scanline < cinfo.output_height)
        {
            unsigned char *buffer_array[1];
            buffer_array[0] = jpeg_decomp + (cinfo.output_scanline) * cinfo.image_width;
            jpeg_read_scanlines(&cinfo, buffer_array, 1);
        }

        // Cleanup
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);

        // Init CImg image
        img = cimg_library::CImg<unsigned short>(cinfo.image_width, cinfo.image_height, 1, 1);

        // Copy over
        for (int i = 0; i < (int)cinfo.image_width * (int)cinfo.image_height; i++)
            img[i] = jpeg_decomp[i];

        // Free memory
        delete[] jpeg_decomp;

        return img;
    }
}