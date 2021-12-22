#include "image.h"
#include <cstring>
#include <png.h>
#include "logger.h"

#define INVERT_ENDIAN_16(x) ((x >> 8) | (x << 8))

namespace image
{
    template <typename T>
    void Image<T>::save_png(std::string file, bool fast)
    {
        if (data_size == 0 || d_height == 0) // Make sure we aren't just gonna crash
        {
            logger->trace("Tried to save empty PNG!");
            return;
        }

        FILE *fp = fopen(file.c_str(), "wb");
        if (!fp)
            abort();

        png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png)
            abort();

        png_infop info = png_create_info_struct(png);
        if (!info)
            abort();

        if (setjmp(png_jmpbuf(png)))
            abort();

        png_init_io(png, fp);

        // Set color format
        int color_type = 0;
        if (d_channels == 1)
            color_type = PNG_COLOR_TYPE_GRAY;
        else if (d_channels == 3)
            color_type = PNG_COLOR_TYPE_RGB;
        else if (d_channels == 4)
            color_type = PNG_COLOR_TYPE_RGBA;

        png_set_IHDR(png, info, d_width, d_height, d_depth, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        if (fast) // Fast-saving, disabling all filters. This has a minor impact on compression perfs but huge on saving speed
            png_set_filter(png, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_NONE);
        png_write_info(png, info);

        // Write row-per-row to save up on memory
        {
            png_byte *const image_row = new png_byte[sizeof(T) * d_channels * d_width];

            if (d_depth == 8)
            {
                for (int row = 0; row < d_height; row++)
                {
                    for (int c = 0; c < d_channels; c++)
                        for (int i = 0; i < d_width; i++)
                            image_row[i * d_channels + c] = channel(c)[row * d_width + i];
                    png_write_row(png, image_row);
                }
            }
            else if (d_depth == 16)
            {
                for (int row = 0; row < d_height; row++)
                {
                    for (int c = 0; c < d_channels; c++)
                        for (int i = 0; i < d_width; i++)
                            ((uint16_t *)image_row)[i * d_channels + c] = INVERT_ENDIAN_16(channel(c)[row * d_width + i]);
                    png_write_row(png, image_row);
                }
            }

            delete[] image_row;
        }

        png_write_end(png, NULL);
        fclose(fp);
        png_destroy_write_struct(&png, &info);
    }

    template <typename T>
    void Image<T>::load_png(std::string file)
    {
        FILE *fp = fopen(file.c_str(), "rb");

        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png)
            abort();

        png_infop info = png_create_info_struct(png);
        if (!info)
            abort();

        if (setjmp(png_jmpbuf(png)))
            abort();

        png_init_io(png, fp);
        png_read_info(png, info);

        d_width = png_get_image_width(png, info);
        d_height = png_get_image_height(png, info);
        int color_type = png_get_color_type(png, info);
        int bit_depth = png_get_bit_depth(png, info);

        if (color_type == PNG_COLOR_TYPE_GRAY)
            d_channels = 1;
        else if (color_type == PNG_COLOR_TYPE_RGB)
            d_channels = 3;
        else if (color_type == PNG_COLOR_TYPE_RGBA)
            d_channels = 4;

        init(d_width, d_height, d_channels);

        // Read row per row, with lower memory usage that reading the whole image!
        {
            png_byte *const image_row = new png_byte[(bit_depth / 8) * d_channels * d_width];

            if (bit_depth == 8)
            {
                // if the image is 8-bits and not 16 while we are 16, shift it up to 16
                int shift = 0;
                if (d_depth == 16)
                    shift = 8;

                for (int row = 0; row < d_height; row++)
                {
                    png_read_row(png, NULL, image_row);
                    for (int c = 0; c < d_channels; c++)
                        for (int i = 0; i < d_width; i++)
                            channel(c)[row * d_width + i] = image_row[i * d_channels + c] << shift;
                }
            }
            else if (bit_depth == 16)
            {
                // if the image is 16-bits and not 8 while we are 16, shift it down to 8
                int shift = 0;
                if (d_depth == 8)
                    shift = 8;

                for (int row = 0; row < d_height; row++)
                {
                    png_read_row(png, NULL, image_row);
                    for (int c = 0; c < d_channels; c++)
                        for (int i = 0; i < d_width; i++)
                            channel(c)[row * d_width + i] = INVERT_ENDIAN_16(((uint16_t *)image_row)[i * d_channels + c]) >> shift;
                }
            }

            delete[] image_row;
        }

        fclose(fp);
        png_destroy_read_struct(&png, &info, NULL);
    }

    // Generate Images for uint16_t and uint8_t
    template class Image<uint8_t>;
    template class Image<uint16_t>;
}