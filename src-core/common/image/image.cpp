#include "image.h"
#include <stdexcept>
extern "C"
{
#include "libs/jpeg/jpeglib.h"
}

namespace image
{
    template <typename T>
    int percentile(T *array, int size, float percentile)
    {
        float number_percent = (size + 1) * percentile / 100.0f;
        if (number_percent == 1)
            return array[0];
        else if (number_percent == size)
            return array[size - 1];
        else
            return array[(int)number_percent - 1] + (number_percent - (int)number_percent) * (array[(int)number_percent] - array[(int)number_percent - 1]);
    }

    template <typename T>
    void white_balance(cimg_library::CImg<T> &image, float percentileValue, int channelCount)
    {
        int height = image.height();
        int width = image.width();
        float maxVal = std::numeric_limits<T>::max();

        T *sorted_array = new T[height * width];

        for (int band_number = 0; band_number < channelCount; band_number++)
        {
            // Load the whole image band into our array
            std::memcpy(sorted_array, &image.data()[band_number * width * height], width * height * sizeof(T));

            // Sort it
            std::sort(&sorted_array[0], &sorted_array[width * height]);

            // Get percentiles
            int percentile1 = percentile(sorted_array, width * height, percentileValue);
            int percentile2 = percentile(sorted_array, width * height, 100.0f - percentileValue);

            for (int i = 0; i < width * height; i++)
            {
                long balanced = (image[band_number * width * height + i] - percentile1) * maxVal / (percentile2 - percentile1);
                if (balanced < 0)
                    balanced = 0;
                else if (balanced > maxVal)
                    balanced = maxVal;
                image[band_number * width * height + i] = balanced;
            }
        }

        delete[] sorted_array;
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

    static void libjpeg_error_func_ignore(j_common_ptr /*cinfo*/)
    {
        //longjmp(((jpeg_error_struct *)cinfo->err)->setjmp_buffer, 1);
    }

    cimg_library::CImg<unsigned char> decompress_jpeg(uint8_t *data, int length, bool ignore_errors)
    {
        cimg_library::CImg<unsigned char> img;
        unsigned char *jpeg_decomp = NULL;

        // Huge thanks to https://gist.github.com/PhirePhly/3080633
        jpeg_error_struct jerr;
        jpeg_decompress_struct cinfo;

        // Init
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = ignore_errors ? libjpeg_error_func_ignore : libjpeg_error_func;

        if (setjmp(jerr.setjmp_buffer))
        {
            // Free memory
            delete[] jpeg_decomp;
            return img;
        }

        jpeg_create_decompress(&cinfo);

        // Parse and start decompressing
        jpeg_mem__src(&cinfo, data, length);
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
        img = cimg_library::CImg<unsigned char>(cinfo.image_width, cinfo.image_height, 1, 1);

        // Copy over
        for (int i = 0; i < (int)cinfo.image_width * (int)cinfo.image_height; i++)
            img[i] = jpeg_decomp[i];

        // Free memory
        delete[] jpeg_decomp;

        return img;
    }

    void simple_despeckle(cimg_library::CImg<unsigned short> &image, int thresold)
    {
        int h = image.height();
        int w = image.width();

        for (int x = 0; x < h; x++)
        {
            for (int y = 0; y < w; y++)
            {
                unsigned short current = image[x * w + y];

                unsigned short below = x + 1 == h ? 0 : image[(x + 1) * w + y];
                unsigned short left = y - 1 == -1 ? 0 : image[x * w + (y - 1)];
                unsigned short right = y + 1 == w ? 0 : image[x * w + (y + 1)];

                if ((current - left > thresold && current - right > thresold) ||
                    (current - below > thresold && current - right > thresold))
                {
                    image[x * w + y] = (right + left) / 2;
                }
            }
        }
    }

    void extract_percentile(cimg_library::CImg<unsigned short> &image, float percentilev1, float percentilev2, int channelCount)
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
            int percentile1 = percentile(sorted_array, width * height, percentilev1);
            int percentile2 = percentile(sorted_array, width * height, percentilev2);

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

        delete[] sorted_array;
    }

    template <typename T>
    void linear_invert(cimg_library::CImg<T> &image)
    {
        float scale = std::numeric_limits<T>::max() - 1;

        for (int i = 0; i < image.width() * image.height(); i++)
        {
            image[i] = scale - image[i];
        }
    }

    void brightness_contrast_old(cimg_library::CImg<unsigned short> &image, float brightness, float contrast, int channelCount)
    {
        float brightness_v = brightness / 2.0f;
        float slant = tanf((contrast + 1.0f) * 0.78539816339744830961566084581987572104929234984378f);

        for (int i = 0; i < image.height() * image.width() * channelCount; i++)
        {
            float v = image[i];

            if (brightness_v < 0.0f)
                v = v * (65535.0f + brightness_v);
            else
                v = v + ((65535.0f - v) * brightness_v);

            v = (v - 32767.5f) * slant + 32767.5f;

            image[i] = std::min<float>(65535, std::max<float>(0, v * 2.0f));
        }
    }

    cimg_library::CImg<unsigned short> generate_LUT(int width, int x0, int x1, cimg_library::CImg<unsigned short> input, bool vertical)
    {
        if (vertical)
            input.rotate(-90);
        input.resize(x1 - x0, 1, 1, 3, 3);
        cimg_library::CImg<unsigned short> out(width, 1, 1, 3);
        std::memset(out, 0, width * 3);
        out.draw_image(x0, 0, input);
        return out;
    }

    cimg_library::CImg<unsigned char> generate_LUT(int width, int x0, int x1, cimg_library::CImg<unsigned char> input, bool vertical)
    {
        if (vertical)
            input.rotate(-90);
        input.resize(x1 - x0, 1, 1, 3, 3);
        cimg_library::CImg<unsigned char> out(width, 1, 1, 3);
        std::memset(out, 0, width * 3);
        out.draw_image(x0, 0, input);
        return out;
    }

    template void linear_invert<unsigned char>(cimg_library::CImg<unsigned char> &);
    template void linear_invert<unsigned short>(cimg_library::CImg<unsigned short> &);

    template void white_balance(cimg_library::CImg<unsigned char> &, float, int);
    template void white_balance(cimg_library::CImg<unsigned short> &, float, int);
}