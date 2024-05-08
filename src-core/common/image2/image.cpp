#include "image.h"
#include <cstring>
#include <cstdlib>
#include "core/exception.h"

namespace image2
{
    Image::Image()
    {
        // Do nothing
    }

    Image::Image(int bit_depth, size_t width, size_t height, int channels)
    {
        init(bit_depth, width, height, channels);
    }

    Image::Image(const Image &img)
    {
        // Copy contents of the image over
        init(img.d_depth, img.d_width, img.d_height, img.d_channels);
        memcpy(d_data, img.d_data, img.data_size * img.type_size);
        //        copy_meta(img);
    }

    Image &Image::operator=(const Image &img)
    {
        // Copy contents of the image over
        init(img.d_depth, img.d_width, img.d_height, img.d_channels);
        memcpy(d_data, img.d_data, img.data_size * img.type_size);
        //        copy_meta(img);
        return *this;
    }

    Image::Image(void *buffer, int bit_depth, size_t width, size_t height, int channels)
    {
        // Copy contents of the image over
        init(bit_depth, width, height, channels);
        memcpy(d_data, buffer, data_size * type_size);
    }

    Image::~Image()
    {
        if (has_data)
            free(d_data);

        //        free_metadata(*this);
    }

    void Image::init(int bit_depth, size_t width, size_t height, int channels)
    {
        // Reset image if we already had one
        if (has_data)
            free(d_data);

        // Internal params
        if (bit_depth > 8)
            type_size = 2;
        else
            type_size = 1;

        // Init buffer
        data_size = width * height * channels;
        d_data = malloc(type_size * data_size);

        // Set to 0
        memset(d_data, 0, type_size * data_size);

        // Init local variables
        d_depth = bit_depth;
        d_maxv = (2 << (d_depth - 1)) - 1;
        d_width = width;
        d_height = height;
        d_channels = channels;

        // We have data now if we didn't already
        has_data = true;
    }

    void Image::clear()
    {
        // Reset image
        if (has_data && d_data != nullptr)
            free(d_data);
        d_data = nullptr;
        has_data = false;
    }

    int Image::clamp(int input)
    {
        if (input > d_maxv)
            return d_maxv;
        else if (input < 0)
            return 0;
        else
            return input;
    }

    double Image::clampf(double input)
    {
        if (input > 1.0)
            return 1.0;
        else if (input < 0)
            return 0;
        else
            return input;
    }

    void Image::to_rgb()
    {
        if (d_channels == 1)
        {
            Image tmp = *this;                   // Backup image
            init(d_depth, d_width, d_height, 3); // Init new image as RGB

            // Fill in all 3 channels
            draw_image(0, tmp);
            draw_image(1, tmp);
            draw_image(2, tmp);
        }
        else if (d_channels == 4)
        {
            Image tmp = *this;                   // Backup image
            init(d_depth, d_width, d_height, 3); // Init new image as RGB

            // Copy over all 3 channels
            memcpy(d_data, tmp.d_data, d_width * d_height * 3 * type_size);
        }
    }

    void Image::to_rgba()
    {
        if (d_channels == 1)
        {
            Image tmp = *this;                   // Backup image
            init(d_depth, d_width, d_height, 4); // Init new image as RGBA

            // Copy over all 3 channels
            memcpy(d_data + type_size * d_width * d_height * 0, tmp.d_data, d_width * d_height * type_size);
            memcpy(d_data + type_size * d_width * d_height * 1, tmp.d_data, d_width * d_height * type_size);
            memcpy(d_data + type_size * d_width * d_height * 2, tmp.d_data, d_width * d_height * type_size);
            for (size_t i = 0; i < d_width * d_height; i++)
                set(3, i, d_maxv);
        }
        else if (d_channels == 2)
        {
            Image tmp = *this;                   // Backup image
            init(d_depth, d_width, d_height, 4); // Init new image as RGBA

            // Copy over all 3 channels
            memcpy(d_data + type_size * d_width * d_height * 0, tmp.d_data, d_width * d_height * type_size);
            memcpy(d_data + type_size * d_width * d_height * 1, tmp.d_data, d_width * d_height * type_size);
            memcpy(d_data + type_size * d_width * d_height * 2, tmp.d_data, d_width * d_height * type_size);

            // Copy over RGBA
            memcpy(d_data + type_size * d_width * d_height * 3, tmp.d_data + d_width * d_height, d_width * d_height * type_size);
        }
        else if (d_channels == 3)
        {
            Image tmp = *this;                   // Backup image
            init(d_depth, d_width, d_height, 4); // Init new image as RGBA

            // Copy over all 3 channels
            memcpy(d_data, tmp.d_data, d_width * d_height * 3 * type_size);

            // Fill in RGBA
            for (size_t i = 0; i < d_width * d_height; i++)
                set(3, i, d_maxv);
        }
    }

    Image Image::to8bits()
    {
        if (d_depth == 8)
        {
            return *this;
        }
        else if (d_depth == 16)
        {
            Image image8(8, d_width, d_height, d_channels);
            for (size_t i = 0; i < data_size; i++)
                image8.set(i, get(i) >> 8);
            return image8;
        }

        throw satdump_exception("Error in to8bits()"); // This should never happen
    }

    Image Image::to16bits()
    {
        if (d_depth == 16)
        {
            return *this;
        }
        else if (d_depth == 8)
        {
            Image image16(16, d_width, d_height, d_channels);
            for (size_t i = 0; i < data_size; i++)
                image16.set(i, get(i) << 8);
            return image16;
        }

        throw satdump_exception("Error in to8bits()"); // This should never happen
    }

    void Image::crop(int x0, int y0, int x1, int y1)
    {
        int new_width = x1 - x0;
        int new_height = y1 - y0;

        // Create new buffer
        void *new_data = malloc(new_width * new_height * d_channels * type_size);

        // Copy cropped area to new region
        for (int c = 0; c < d_channels; c++)
            for (int x = 0; x < new_width; x++)
                for (int y = 0; y < new_height; y++)
                    memcpy(new_data + ((new_width * new_height * c) + y * new_width + x) * type_size,
                           d_data + (c * d_width * d_height + (y0 + y) * d_width + (x + x0)) * type_size,
                           type_size);

        // Swap out buffer
        free(d_data);
        d_data = new_data;

        // Update info
        data_size = new_width * new_height * d_channels;
        d_width = new_width;
        d_height = new_height;
    }

    void Image::crop(int x0, int x1)
    {
        crop(x0, 0, x1, d_height);
    }

    Image Image::crop_to(int x0, int y0, int x1, int y1)
    {
        int new_width = x1 - x0;
        int new_height = y1 - y0;

        // Create new buffer
        Image new_data(d_depth, new_width, new_height, d_channels);

        // Copy cropped area to new region
        for (int c = 0; c < d_channels; c++)
            for (int x = 0; x < new_width; x++)
                for (int y = 0; y < new_height; y++)
                    new_data.set(c, x, y, get(c, (x + x0), (y0 + y)));

        return new_data;
    }

    Image Image::crop_to(int x0, int x1)
    {
        return crop_to(x0, 0, x1, d_height);
    }

    void Image::mirror(bool x, bool y)
    {
        if (y) // Mirror on the Y axis
        {
            int *tmp_col = (int *)malloc(d_height * sizeof(int));

            for (int c = 0; c < d_channels; c++)
            {
                for (size_t col = 0; col < d_width; col++)
                {
                    for (size_t i = 0; i < d_height; i++) // Buffer column
                        tmp_col[i] = get(c, col, i);

                    for (size_t i = 0; i < d_height; i++) // Restore and mirror
                        set(c, col, i, tmp_col[(d_height - 1) - i]);
                }
            }

            free(tmp_col);
        }

        if (x) // Mirror on the X axis
        {
            int *tmp_row = (int *)malloc(d_width * sizeof(int));

            for (int c = 0; c < d_channels; c++)
            {
                for (size_t row = 0; row < d_height; row++)
                {
                    for (size_t i = 0; i < d_width; i++) // Buffer column
                        tmp_row[i] = get(c, i, row);

                    for (size_t i = 0; i < d_width; i++) // Restore and mirror
                        set(c, i, row, tmp_row[(d_width - 1) - i]);
                }
            }

            free(tmp_row);
        }
    }

}