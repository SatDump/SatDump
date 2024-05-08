#include "image.h"
#include "core/exception.h"

namespace image2
{
    void Image::draw_image(int c, Image image, int x0, int y0)
    {
        if (image.d_depth != d_depth)
            throw satdump_exception("draw_image bit depth must be the same!");

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

    void Image::draw_pixel(size_t x, size_t y, std::vector<double> color)
    {
        if (color.size() < d_channels)
            throw satdump_exception("draw_pixel color needs to have at least as many colors as the image!");

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
}