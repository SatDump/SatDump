#include "image.h"
#include <cstdlib>
#include "core/exception.h"
#include "meta.h"

#include <cmath>

namespace image
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
        copy_meta(img);
    }

    Image &Image::operator=(const Image &img)
    {
        if (img.d_data != nullptr)
        {
            // Copy contents of the image over
            init(img.d_depth, img.d_width, img.d_height, img.d_channels);
            memcpy(d_data, img.d_data, img.data_size * img.type_size);
        }
        copy_meta(img);
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
        if (d_data != nullptr)
        {
            free(d_data);
            d_data = nullptr;
        }

        free_metadata(*this);
    }

    void Image::init(int bit_depth, size_t width, size_t height, int channels)
    {
        // Reset image if we already had one
        if (d_data != nullptr)
        {
            free(d_data);
            d_data = nullptr;
        }

        // Internal params
        if (bit_depth > 8)
            type_size = 2;
        else
            type_size = 1;

        // Init buffer
        data_size = width * height * channels;
        d_data = malloc(type_size * data_size);
        if (d_data == NULL)
            throw satdump_exception("Could not allocate memory for image!");

        // Set to 0
        memset(d_data, 0, type_size * data_size);

        // Init local variables
        d_depth = bit_depth;
        d_maxv = (2 << (d_depth - 1)) - 1;
        d_width = width;
        d_height = height;
        d_channels = channels;
    }

    void Image::clear()
    {
        // Reset image
        if (d_data != nullptr)
            free(d_data);
        d_data = nullptr;
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
            memcpy((uint8_t *)d_data + type_size * d_width * d_height * 0, tmp.d_data, d_width * d_height * type_size);
            memcpy((uint8_t *)d_data + type_size * d_width * d_height * 1, tmp.d_data, d_width * d_height * type_size);
            memcpy((uint8_t *)d_data + type_size * d_width * d_height * 2, tmp.d_data, d_width * d_height * type_size);
            for (size_t i = 0; i < d_width * d_height; i++)
                set(3, i, d_maxv);
        }
        else if (d_channels == 2)
        {
            Image tmp = *this;                   // Backup image
            init(d_depth, d_width, d_height, 4); // Init new image as RGBA

            // Copy over all 3 channels
            memcpy((uint8_t *)d_data + type_size * d_width * d_height * 0, tmp.d_data, d_width * d_height * type_size);
            memcpy((uint8_t *)d_data + type_size * d_width * d_height * 1, tmp.d_data, d_width * d_height * type_size);
            memcpy((uint8_t *)d_data + type_size * d_width * d_height * 2, tmp.d_data, d_width * d_height * type_size);

            // Copy over RGBA
            memcpy((uint8_t *)d_data + type_size * d_width * d_height * 3, (uint8_t *)tmp.d_data + d_width * d_height, d_width * d_height * type_size);
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

    Image Image::to_depth(int bit_depth)
    {
        if (bit_depth > 8)
            return to16bits();
        else
            return to8bits();
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
                    memcpy((uint8_t *)new_data + ((new_width * new_height * c) + y * new_width + x) * type_size,
                           (uint8_t *)d_data + (c * d_width * d_height + (y0 + y) * d_width + (x + x0)) * type_size,
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

    void Image::resize(int width, int height)
    {
        double x_scale = double(d_width) / double(width);
        double y_scale = double(d_height) / double(height);

        Image tmp = *this;
        init(d_depth, width, height, d_channels);

        for (int c = 0; c < d_channels; c++)
        {
            for (size_t x = 0; x < d_width; x++)
            {
                for (size_t y = 0; y < d_height; y++)
                {
                    int xx = floor(double(x) * x_scale);
                    int yy = floor(double(y) * y_scale);

                    set(c, x, y, tmp.get(c, xx, yy));
                }
            }
        }
    }

    Image Image::resize_to(int width, int height)
    {
        double x_scale = double(d_width) / double(width);
        double y_scale = double(d_height) / double(height);

        Image ret(d_depth, width, height, d_channels);

        for (int c = 0; c < d_channels; c++)
        {
            for (size_t x = 0; x < (size_t)width; x++)
            {
                for (size_t y = 0; y < (size_t)height; y++)
                {
                    int xx = floor(double(x) * x_scale);
                    int yy = floor(double(y) * y_scale);

                    ret.set(c, x, y, get(c, xx, yy));
                }
            }
        }

        return ret;
    }

    void Image::resize_bilinear(int width, int height, bool text_mode)
    {
        int a = 0, b = 0, c = 0, d = 0, x = 0, y = 0;
        size_t index;
        double x_scale = double(d_width - 1) / double(width);
        double y_scale = double(d_height - 1) / double(height);
        float x_diff, y_diff, val;

        Image tmp = *this;
        init(d_depth, width, height, d_channels);
        size_t max_index = tmp.width() * tmp.height();

        for (int cc = 0; cc < d_channels; cc++)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    x = (int)(x_scale * j);
                    y = (int)(y_scale * i);

                    x_diff = (x_scale * j) - x;
                    y_diff = (y_scale * i) - y;

                    index = (y * tmp.width() + x);

                    a = tmp.get(cc, index);
                    if (index + 1 < max_index)
                        b = tmp.get(cc, index + 1);
                    if (index + tmp.width() < max_index)
                        c = tmp.get(cc, index + tmp.width());
                    if (index + tmp.width() + 1 < max_index)
                        d = tmp.get(cc, index + tmp.width() + 1);

                    val = a * (1 - x_diff) * (1 - y_diff) +
                          b * (x_diff) * (1 - y_diff) +
                          c * (y_diff) * (1 - x_diff) +
                          d * (x_diff * y_diff);

                    if (text_mode) // Special text mode, where we want to keep it clear whatever the res is
                        set(cc, i * width + j, val > 0 ? d_maxv : 0);
                    else
                        set(cc, i * width + j, val);
                }
            }
        }
    }

    int Image::get_pixel_bilinear(int cc, double rx, double ry)
    {
        size_t x = (size_t)rx;
        size_t y = (size_t)ry;

        double x_diff = rx - x;
        double y_diff = ry - y;

        size_t index = (y * d_width + x);
        size_t max_index = d_width * d_height;

        int a = 0, b = 0, c = 0, d = 0;
        float a_a = 1.0f, b_a = 1.0f, c_a = 1.0f, d_a = 1.0f;

        a = get(cc, index);
        if (d_channels == 4 && cc != 3)
            a_a = (float)get(3, index) / (float)d_maxv;

        if (index + 1 < max_index)
        {
            b = get(cc, index + 1);
            if (d_channels == 4 && cc != 3)
            {
                b_a = (float)get(3, index + 1) / (float)d_maxv;
                b = (float)b * b_a;
            }
        }
        else
            return a;

        if (index + d_width < max_index)
        {
            c = get(cc, index + d_width);
            if (d_channels == 4 && cc != 3)
            {
                c_a = (float)get(3, index + d_width) / (float)d_maxv;
                c = (float)c * c_a;
            }
        }
        else
            return a;

        if (index + d_width + 1 < max_index)
        {
            d = get(cc, index + d_width + 1);
            if (d_channels == 4 && cc != 3)
            {
                d_a = (float)get(3, index + d_width + 1) / (float)d_maxv;
                d = (float)d * d_a;
            }
        }
        else
            return a;

        if (x == d_width - 1)
            return a;
        if (y == d_height - 1)
            return a;

        a = (float)a * a_a;
        int ret = clamp(a * (1 - x_diff) * (1 - y_diff) +
                        b * (x_diff) * (1 - y_diff) +
                        c * (y_diff) * (1 - x_diff) +
                        d * (x_diff * y_diff));
        if (d_channels == 4 && cc != 3)
        {
            ret = (float)ret / (a_a * (1 - x_diff) * (1 - y_diff) +
                                b_a * (x_diff) * (1 - y_diff) +
                                c_a * (y_diff) * (1 - x_diff) +
                                d_a * (x_diff * y_diff));
        }

        return ret;
    }

    void Image::fill(int val)
    {
        for (int c = 0; c < d_channels; c++)
            for (size_t i = 0; i < d_width * d_height; i++)
                set(c, i, val);
    }

    void Image::fill_color(std::vector<double> color)
    {
        for (size_t x = 0; x < d_width; x++)
            for (size_t y = 0; y < d_height; y++)
                draw_pixel(x, y, color);
    }

    ////////////////////////

    void imemcpy(Image &img1, size_t pos1, Image &img2, size_t pos2, size_t px_size)
    {
        if (img1.depth() != img2.depth())
            throw satdump_exception("image::memcpy both images must be the same bit depth!");
        if (pos1 + px_size > img1.size())
            throw satdump_exception("image::memcpy pos1 + px_size exceeds img1 size!");
        if (pos2 + px_size > img2.size())
            throw satdump_exception("image::memcpy pos2 + px_size exceeds img2 size!");

        memcpy((uint8_t *)img1.raw_data() + pos1 * img1.typesize(),
               (uint8_t *)img2.raw_data() + pos2 * img1.typesize(),
               px_size * img1.typesize());
    }

    void image_to_rgba(Image &img, uint32_t *output)
    {
        int shift = img.depth() - 8;
        if (img.channels() == 1)
        {
            for (size_t i = 0; i < img.width() * img.height(); i++)
            {
                uint8_t c;
                c = img.get(i) >> shift;
                output[i] = ((uint32_t)255 << 24) | ((uint32_t)c << 16) | ((uint32_t)c << 8) | (uint32_t)c;
            }
        }
        else if (img.channels() == 2)
        {
            for (size_t i = 0; i < img.width() * img.height(); i++)
            {
                uint8_t r, g, b, a;
                r = img.get(0, i) >> shift;
                g = img.get(0, i) >> shift;
                b = img.get(0, i) >> shift;
                a = img.get(1, i) >> shift;
                output[i] = ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r;
            }
        }
        else if (img.channels() == 3)
        {
            for (size_t i = 0; i < img.width() * img.height(); i++)
            {
                uint8_t r, g, b;
                r = img.get(0, i) >> shift;
                g = img.get(1, i) >> shift;
                b = img.get(2, i) >> shift;
                output[i] = ((uint32_t)255 << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r;
            }
        }
        else if (img.channels() == 4)
        {
            for (size_t i = 0; i < img.width() * img.height(); i++)
            {
                uint8_t r, g, b, a;
                r = img.get(0, i) >> shift;
                g = img.get(1, i) >> shift;
                b = img.get(2, i) >> shift;
                a = img.get(3, i) >> shift;
                output[i] = ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r;
            }
        }
    }
}