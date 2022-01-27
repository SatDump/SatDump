#include "image.h"
#include <cstring>
#include <cmath>
#include <limits>
#include <vector>
#include "resources.h"

namespace image
{
    template <typename T>
    void Image<T>::draw_pixel(int x, int y, T color[])
    {
        // Check we're not out of bounds
        if (x < 0 || y < 0 || x >= d_width || y >= d_height)
            return;

        for (int c = 0; c < d_channels; c++)
            channel(c)[y * d_width + x] = color[c];
    }

    template <typename T>
    void Image<T>::draw_line(int x0, int y0, int x1, int y1, T color[])
    {
        int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = (dx > dy ? dx : -dy) / 2, e2;

        while (!(x0 == x1 && y0 == y1))
        {
            draw_pixel(x0, y0, color);

            e2 = err;

            if (e2 > -dx)
            {
                err -= dy;
                x0 += sx;
            }

            if (e2 < dy)
            {
                err += dx;
                y0 += sy;
            }
        }
    }

    template <typename T>
    void Image<T>::draw_circle(int x0, int y0, int radius, T color[], bool fill)
    {
        if (fill) // Filled circle
        {
            int x = radius;
            int y = 0;
            int err = 1 - x;

            while (x >= y)
            {
                draw_line(-x + x0, y + y0, x + x0, y + y0, color);
                if (y != 0)
                    draw_line(-x + x0, -y + y0, x + x0, -y + y0, color);

                y++;

                if (err < 0)
                    err += 2 * y + 1;
                else
                {
                    if (x >= y)
                    {
                        draw_line(-y + 1 + x0, x + y0, y - 1 + x0, x + y0, color);
                        draw_line(-y + 1 + x0, -x + y0, y - 1 + x0, -x + y0, color);
                    }

                    x--;
                    err += 2 * (y - x + 1);
                }
            }
        }
        else // Hollow circle
        {
            int err = 1 - radius;
            int xx = 0;
            int yy = -2 * radius;
            int x = 0;
            int y = radius;

            draw_pixel(x0, y0 + radius, color);
            draw_pixel(x0, y0 - radius, color);
            draw_pixel(x0 + radius, y0, color);
            draw_pixel(x0 - radius, y0, color);

            while (x < y)
            {
                if (err >= 0)
                {
                    y--;
                    yy += 2;
                    err += yy;
                }

                x++;
                xx += 2;
                err += xx + 1;

                draw_pixel(x0 + x, y0 + y, color);
                draw_pixel(x0 - x, y0 + y, color);
                draw_pixel(x0 + x, y0 - y, color);
                draw_pixel(x0 - x, y0 - y, color);
                draw_pixel(x0 + y, y0 + x, color);
                draw_pixel(x0 - y, y0 + x, color);
                draw_pixel(x0 + y, y0 - x, color);
                draw_pixel(x0 - y, y0 - x, color);
            }
        }
    }

    template <typename T>
    void Image<T>::draw_image(int c, Image<T> image, int x0, int y0)
    {
        // Get min height and width, mostly for safety
        int width = std::min<int>(d_width, x0 + image.width()) - x0;
        int height = std::min<int>(d_height, y0 + image.height()) - y0;

        for (int x = 0; x < width; x++)
            for (int y = 0; y < height; y++)
                channel(c)[(y + y0) * d_width + x + x0] = image[y * image.width() + x];

        if (c == 0 && image.channels() == d_channels) // Special case for non-grayscale images
        {
            for (int c = 1; c < d_channels; c++)
                for (int x = 0; x < width; x++)
                    for (int y = 0; y < height; y++)
                        channel(c)[(y + y0) * d_width + x + x0] = image.channel(c)[y * image.width() + x];
        }
    }

    std::vector<Image<uint8_t>> make_font(int size, bool text_mode)
    {
        Image<uint8_t> fontFile;
        fontFile.load_png(resources::getResourcePath("fonts/Roboto-Regular.png"));

        std::vector<Image<uint8_t>> font;

        int char_size = 120;
        int char_count = 95;
        int char_edge_crop = 15;

        for (int i = 0; i < char_count; i++)
        {
            Image<uint8_t> char_img = fontFile;
            char_img.crop(i * char_size, 0, (i + 1) * char_size, char_size);
            char_img.crop(char_edge_crop, char_img.width() - char_edge_crop);
            char_img.resize_bilinear((float)char_img.width() * ((float)size / (float)char_size), size, text_mode);
            font.push_back(char_img);
        }

        return font;
    }

    template <typename T>
    void Image<T>::draw_text(int x0, int y0, T color[], std::vector<Image<uint8_t>> font, std::string text)
    {
        int pos = x0;
        T *colorf = new T[d_channels];

        for (char character : text)
        {
            if (size_t(character - 31) > font.size())
                continue;

            Image<uint8_t> &img = font[character - 31];

            for (int x = 0; x < img.width(); x++)
            {
                for (int y = 0; y < img.height(); y++)
                {
                    float value = img[y * img.width() + x] / 255.0f;

                    if (value == 0)
                        continue;

                    for (int c = 0; c < d_channels; c++)
                        colorf[c] = color[c] * value;

                    draw_pixel(pos + x, y0 + y, colorf);
                }
            }

            pos += img.width();
        }

        delete[] colorf;
    }

    template <typename T>
    Image<T> generate_text_image(std::string text, T color[], int height, int padX, int padY){
        std::vector<Image<uint8_t>> font = make_font(height-2*padY, false);
        int width = font[0].width()*text.length() + 2*padX;
        Image<T> out(width, height, 1);
        out.fill(0);
        out.draw_text(padX, 0, color, font, text);
        return out;
    }

    template Image<uint8_t> generate_text_image(std::string text, uint8_t color[], int height, int padX, int padY);
    template Image<uint16_t> generate_text_image(std::string text, uint16_t color[], int height, int padX, int padY);

    // Generate Images for uint16_t and uint8_t
    template class Image<uint8_t>;
    template class Image<uint16_t>;
}
