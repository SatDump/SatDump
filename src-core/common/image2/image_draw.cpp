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
}