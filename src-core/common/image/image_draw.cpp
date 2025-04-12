#include "image.h"
#include "core/exception.h"

namespace image
{
    void Image::draw_image(int c, Image image, int x0, int y0)
    {
        if (image.d_depth != d_depth)
            throw satdump_exception("draw_image bit depth must be the same! " + std::to_string(d_depth) + " != " + std::to_string(image.depth()));

        // Get min height and width, mostly for safety
        int width = std::min<int>(d_width, x0 + image.width()) - x0;
        int height = std::min<int>(d_height, y0 + image.height()) - y0;

        for (int x = 0; x < width; x++)
            for (int y = 0; y < height; y++)
                if (y + y0 >= 0 && x + x0 >= 0)
                    set(c, x + x0, (y + y0), image.get(0, x, y));

        if (c == 0 && image.channels() == d_channels) // Special case for non-grayscale images
        {
            for (int c = 1; c < d_channels; c++)
                for (int x = 0; x < width; x++)
                    for (int y = 0; y < height; y++)
                        if (y + y0 >= 0 && x + x0 >= 0)
                            set(c, x + x0, (y + y0), image.get(c, x, y));
        }
    }

    void Image::draw_image_alpha(Image image, int x0, int y0)
    {
        if (image.d_depth != d_depth)
            throw satdump_exception("draw_image_alpha bit depth must be the same!");
        if (image.channels() != 2 && image.channels() != 4)
            throw satdump_exception("draw_image_alpha input channel count must be 2 or 4!");

        // Get min height and width, mostly for safety
        int width = std::min<int>(d_width, x0 + image.width()) - x0;
        int height = std::min<int>(d_height, y0 + image.height()) - y0;

        double a = 0;
        if (image.channels() == 2 && d_channels <= 2)
        {
            for (int x = 0; x < width; x++)
            {
                for (int y = 0; y < height; y++)
                {
                    if (y + y0 >= 0 && x + x0 >= 0)
                    {
                        a = image.getf(1, x, y);
                        a = image.get(0, x, y) * a + (1.0 - a) * get(0, x + x0, (y + y0));
                        set(0, x + x0, (y + y0), clamp(a));
                    }
                }
            }
        }
        else if (image.channels() == 4 && d_channels >= 3)
        {
            for (int c = 0; c < 3; c++)
            {
                for (int x = 0; x < width; x++)
                {
                    for (int y = 0; y < height; y++)
                    {
                        if (y + y0 >= 0 && x + x0 >= 0)
                        {
                            a = image.getf(3, x, y);
                            a = image.get(c, x, y) * a + (1.0 - a) * get(c, x + x0, (y + y0));
                            set(c, x + x0, (y + y0), clamp(a));
                        }
                    }
                }
            }
        }
        else
            throw satdump_exception("draw_image_alpha has an invalid input/output configuration!");
    }

    void Image::draw_pixel(size_t x, size_t y, std::vector<double> color)
    {
        if (color.size() < (size_t)d_channels)
            throw satdump_exception("draw_pixel color needs to have at least as many colors as the image! C " + std::to_string(color.size()) + " I " + std::to_string(d_channels));

        for (int c = 0; c < d_channels; c++)
            setf(c, x, y, color[c]);
    }

    void Image::draw_line(int x0, int y0, int x1, int y1, std::vector<double> color)
    {
        if (x0 < 0 || x0 >= (int)d_width)
            return;
        if (x1 < 0 || x1 >= (int)d_width)
            return;
        if (y0 < 0 || y0 >= (int)d_height)
            return;
        if (y1 < 0 || y1 >= (int)d_height)
            return;

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

    void Image::draw_circle(int x0, int y0, int radius, std::vector<double> color, bool fill)
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

    void Image::draw_rectangle(int x0, int y0, int x1, int y1, std::vector<double> color, bool fill)
    {
        if (fill)
        {
            for (int y = std::min(y0, y1); y < std::max(y0, y1); y++)
                draw_line(x0, y, x1, y, color);
        }
        else
        {
            draw_line(x0, y0, x0, y1, color);
            draw_line(x0, y1, x1, y1, color);
            draw_line(x1, y1, x1, y0, color);
            draw_line(x1, y0, x0, y0, color);
        }
    }
}