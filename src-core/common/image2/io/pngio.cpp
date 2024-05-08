#include "io.h"
#include <cstring>
#include <png.h>
#include "logger.h"
#include <filesystem>

#define INVERT_ENDIAN_16(x) ((x >> 8) | (x << 8))

namespace image2
{
    void save_png(Image &img, std::string file, bool fast)
    {
        auto d_depth = img.depth();
        auto d_channels = img.channels();
        auto d_height = img.height();
        auto d_width = img.width();

        if (img.size() == 0 || d_height == 0) // Make sure we aren't just gonna crash
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
            png_byte *const image_row = new png_byte[img.typesize() * d_channels * d_width];
            memset(image_row, 0, img.typesize() * d_channels * d_width);

            if (d_depth == 8)
            {
                for (size_t row = 0; row < d_height; row++)
                {
                    for (int c = 0; c < d_channels; c++)
                        for (size_t i = 0; i < d_width; i++)
                            image_row[i * d_channels + c] = img.get(c, i, row);
                    png_write_row(png, image_row);
                }
            }
            else if (d_depth == 16)
            {
                for (size_t row = 0; row < d_height; row++)
                {
                    for (int c = 0; c < d_channels; c++)
                        for (size_t i = 0; i < d_width; i++)
                            ((uint16_t *)image_row)[i * d_channels + c] = INVERT_ENDIAN_16(img.get(c, i, row));
                    png_write_row(png, image_row);
                }
            }

            delete[] image_row;
        }

        png_write_end(png, NULL);
        fclose(fp);
        png_destroy_write_struct(&png, &info);
    }

    void load_png(Image &img, std::string file, bool disableIndexing)
    {
        if (!std::filesystem::exists(file))
            return;

        FILE *fp = fopen(file.c_str(), "rb");

        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png)
        {
            fclose(fp);
            return;
        }

        png_infop info = png_create_info_struct(png);
        if (!info)
        {
            png_destroy_read_struct(&png, NULL, NULL);
            fclose(fp);
            return;
        }

        if (setjmp(png_jmpbuf(png)))
        {
            png_destroy_read_struct(&png, &info, NULL);
            fclose(fp);
            return;
        }

        png_init_io(png, fp);
        png_read_info(png, info);

        size_t d_width = png_get_image_width(png, info);
        size_t d_height = png_get_image_height(png, info);
        int color_type = png_get_color_type(png, info);
        int bit_depth = png_get_bit_depth(png, info);

        int d_channels = 0;
        if (color_type == PNG_COLOR_TYPE_GRAY)
            d_channels = 1;
        else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
            d_channels = 2;
        else if (color_type == PNG_COLOR_TYPE_RGB)
            d_channels = 3;
        else if (color_type == PNG_COLOR_TYPE_RGBA)
            d_channels = 4;
        else if (color_type == PNG_COLOR_TYPE_PALETTE)
        {
            if (!disableIndexing)
            {
                png_set_palette_to_rgb(png);
                d_channels = 3;
            }
            else
                d_channels = 1;
        }

        img.init(bit_depth, d_width, d_height, d_channels);

        // Read row per row, with lower memory usage that reading the whole image!
        {
            int bd = 1;
            if (bit_depth == 16)
                bd = 2;

            png_byte *const image_row = new png_byte[bd * d_channels * d_width];

            if (bit_depth == 8 || color_type == PNG_COLOR_TYPE_PALETTE)
            {
                for (size_t row = 0; row < d_height; row++)
                {
                    png_read_row(png, image_row, NULL);
                    for (int c = 0; c < d_channels; c++)
                        for (size_t i = 0; i < d_width; i++)
                            img.set(c, i, row, image_row[i * d_channels + c]);
                }
            }
            else if (bit_depth == 16)
            {
                for (size_t row = 0; row < d_height; row++)
                {
                    png_read_row(png, NULL, image_row);
                    for (int c = 0; c < d_channels; c++)
                        for (size_t i = 0; i < d_width; i++)
                            img.set(c, i, row, INVERT_ENDIAN_16(((uint16_t *)image_row)[i * d_channels + c]));
                }
            }

            delete[] image_row;
        }

        //   if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        //       to_rgba(); //IMGTODO

        fclose(fp);
        png_destroy_read_struct(&png, &info, NULL);
    }

    struct png_src
    {
        uint8_t *ptr;
        int size;
        int ind;

        static void read(png_structp png_ptr, png_bytep dst, png_size_t size)
        {
            png_voidp io_ptr = png_get_io_ptr(png_ptr);
            png_src *png_this = (png_src *)io_ptr;
            if (png_this->ind >= png_this->size)
                return;
            int rsize = std::min<int>(size, png_this->size - png_this->ind);
            memcpy(dst, png_this->ptr + png_this->ind, rsize);
            png_this->ind += rsize;
        }
    };

    void load_png(Image &img, uint8_t *buffer, int size, bool disableIndexing)
    {
        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png)
            return;

        png_infop info = png_create_info_struct(png);
        if (!info)
        {
            png_destroy_read_struct(&png, NULL, NULL);
            return;
        }

        if (setjmp(png_jmpbuf(png)))
        {
            png_destroy_read_struct(&png, &info, NULL);
            return;
        }

        png_src png_io_v{buffer, size, 0};
        png_set_read_fn(png, &png_io_v, png_src::read);

        // png_init_io(png, fp);
        png_read_info(png, info);

        size_t d_width = png_get_image_width(png, info);
        size_t d_height = png_get_image_height(png, info);
        int color_type = png_get_color_type(png, info);
        int bit_depth = png_get_bit_depth(png, info);

        int d_channels = 0;
        if (color_type == PNG_COLOR_TYPE_GRAY)
            d_channels = 1;
        else if (color_type == PNG_COLOR_TYPE_RGB)
            d_channels = 3;
        else if (color_type == PNG_COLOR_TYPE_RGBA)
            d_channels = 4;
        else if (color_type == PNG_COLOR_TYPE_PALETTE)
        {
            if (!disableIndexing)
            {
                png_set_palette_to_rgb(png);
                d_channels = 3;
            }
            else
                d_channels = 1;
        }

        img.init(bit_depth, d_width, d_height, d_channels);

        // Read row per row, with lower memory usage that reading the whole image!
        {
            int bd = 1;
            if (bit_depth == 16)
                bd = 2;

            png_byte *const image_row = new png_byte[bd * d_channels * d_width];

            if (bit_depth == 8 || color_type == PNG_COLOR_TYPE_PALETTE)
            {
                for (size_t row = 0; row < d_height; row++)
                {
                    png_read_row(png, NULL, image_row);
                    for (int c = 0; c < d_channels; c++)
                        for (size_t i = 0; i < d_width; i++)
                            img.set(c, i, row, image_row[i * d_channels + c]);
                }
            }
            else if (bit_depth == 16)
            {
                for (size_t row = 0; row < d_height; row++)
                {
                    png_read_row(png, NULL, image_row);
                    for (int c = 0; c < d_channels; c++)
                        for (size_t i = 0; i < d_width; i++)
                            img.set(c, i, row, INVERT_ENDIAN_16(((uint16_t *)image_row)[i * d_channels + c]));
                }
            }

            delete[] image_row;
        }

        png_destroy_read_struct(&png, &info, NULL);
    }
}