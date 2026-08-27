#include "simple_mean_filter.h"
#include <cmath>

namespace satdump
{
    namespace image
    {
        float mean_calc(image::Image &img, int width, int height, int ch, int x, int y, int radius)
        {
            int xmin = std::max(x - radius, 0);
            int ymin = std::max(y - radius, 0);
            int xmax = std::min(x + radius, width);
            int ymax = std::min(y + radius, height);

            float val = 0;

            for (int x = xmin; x < xmax; x++)
                for (int y = ymin; y < ymax; y++)
                    val += img.getf(ch, x, y);

            return val / float(abs(xmax - xmin) * abs(ymax - ymin));
        }

        void simple_mean_filter(Image &img, int radius, simple_mean_filter_mode_t mode)
        {
            int width = img.width();
            int height = img.height();

            auto img2 = img;

            for (int c = 0; c < img.channels(); c++)
            {
                for (int x = 0; x < width; x++)
                {
                    for (int y = 0; y < height; y++)
                    {
                        float mean = mean_calc(img, width, height, c, x, y, radius);

                        if (mode == SMOOTH)
                            img2.setf(c, x, y, mean);
                        else if (mode == SHARPEN)
                            img2.setf(c, x, y, img.clampf(img.getf(c, x, y) + (img.getf(c, x, y) - mean)));
                        else if (mode == EDGE)
                            img2.setf(c, x, y, img.getf(c, x, y) - mean);
                    }
                }
            }

            img = img2;
        }
    } // namespace image
} // namespace satdump