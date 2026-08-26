#include "adaptive_despeckle.h"
#include <cmath>
#include <cstddef>

namespace satdump
{
    namespace image
    {
        namespace
        {
            uint8_t pixel_luminance(image::Image &img, int pos)
            {
                if (img.channels() <= 2)
                    return img.getf(pos) * 255;
                else if (img.channels() >= 3)
                    return (img.getf(0, pos) * 255) * 0.22248840 + (img.getf(1, pos) * 255) * 0.71690369 + (img.getf(2, pos) * 255) * 0.06060791;
                return 0;
            }

            struct DespeckleHistogram
            {
                // Number of pixels that fall into each luma bucket
                int elems[256];

                // Original pixels
                std::vector<int> origs[256];

                // Square bracket
                int xmin;
                int ymin;
                int xmax;
                int ymax;

                // Other utils
                int hist0;   // Less than min threshold
                int hist255; // More than max threshold
                int histrest;

                // Functions
                void add(uint8_t val, int pos)
                {
                    elems[val]++;
                    origs[val].push_back(pos);
                }

                void del(uint8_t val)
                {
                    elems[val]--;
                    if (origs[val].size())
                        origs[val].erase(origs[val].begin());
                }

                void clean()
                {
                    for (int i = 0; i < 256; i++)
                    {
                        elems[i] = 0;
                        origs[i].clear();
                    }
                }

                int get_median(int def)
                {
                    int count = histrest;
                    int sum = 0;

                    if (!count)
                        return def;

                    count = (count + 1) / 2;

                    int i = 0;
                    while ((sum += elems[i]) < count)
                        i++;

                    // Return random
                    if (origs[i].size() <= 0)
                        return def;
                    return origs[i][rand() % origs[i].size()];
                }

                // More functions
                void add_val(image::Image &img, int black_level, int white_level, int width, int x, int y)
                {
                    const int pos = (x + (y * width));
                    const int value = pixel_luminance(img, pos);

                    if (value > black_level && value < white_level)
                    {
                        add(value, pos);
                        histrest++;
                    }
                    else
                    {
                        if (value <= black_level)
                            hist0++;

                        if (value >= white_level)
                            hist255++;
                    }
                }

                void del_val(image::Image &img, int black_level, int white_level, int width, int x, int y)
                {
                    const int pos = (x + (y * width));
                    const int value = pixel_luminance(img, pos);

                    if (value > black_level && value < white_level)
                    {
                        del(value);
                        histrest--;
                    }
                    else
                    {
                        if (value <= black_level)
                            hist0--;

                        if (value >= white_level)
                            hist255--;
                    }
                }

                void add_vals(image::Image &img, int black_level, int white_level, int width, int xmin, int ymin, int xmax, int ymax)
                {
                    if (xmin > xmax)
                        return;

                    for (int y = ymin; y <= ymax; y++)
                        for (int x = xmin; x <= xmax; x++)
                            add_val(img, black_level, white_level, width, x, y);
                }

                void del_vals(image::Image &img, int black_level, int white_level, int width, int xmin, int ymin, int xmax, int ymax)
                {
                    if (xmin > xmax)
                        return;

                    for (int y = ymin; y <= ymax; y++)
                        for (int x = xmin; x <= xmax; x++)
                            del_val(img, black_level, white_level, width, x, y);
                }

                void update_histogram(image::Image &img, int black_level, int white_level, int width, int xmin, int ymin, int xmax, int ymax)
                {
                    // assuming that radious of the box can change no more than one pixel in each call
                    // assuming that box is moving either right or down

                    del_vals(img, black_level, white_level, width, this->xmin, this->ymin, xmin - 1, this->ymax);
                    del_vals(img, black_level, white_level, width, xmin, this->ymin, xmax, ymin - 1);
                    del_vals(img, black_level, white_level, width, xmin, ymax + 1, xmax, this->ymax);

                    add_vals(img, black_level, white_level, width, this->xmax + 1, ymin, xmax, ymax);
                    add_vals(img, black_level, white_level, width, xmin, ymin, this->xmax, this->ymin - 1);
                    add_vals(img, black_level, white_level, width, this->xmin, this->ymax + 1, this->xmax, ymax);

                    this->xmin = xmin;
                    this->ymin = ymin;
                    this->xmax = xmax;
                    this->ymax = ymax;
                }
            };
        } // namespace

        void adaptive_despeckle(image::Image &img, AdaptiveDespeckleConfig cfg, float *progress)
        {
            // Performance
            int width = img.width();
            int height = img.height();
            int adapt_radius = cfg.radius;

            DespeckleHistogram histogram;

            image::Image dsti = img;

            for (int y = 0; y < height; y++)
            {
                int x = 0;
                int ymin = std::max(0, y - adapt_radius);
                int ymax = std::min(height - 1, y + adapt_radius);
                int xmin = std::max(0, x - adapt_radius);
                int xmax = std::min(width - 1, x + adapt_radius);

                histogram.hist0 = 0;
                histogram.histrest = 0;
                histogram.hist255 = 0;
                histogram.clean();
                histogram.xmin = xmin;
                histogram.ymin = ymin;
                histogram.xmax = xmax;
                histogram.ymax = ymax;

                histogram.add_vals(img, cfg.black_level, cfg.white_level, width, histogram.xmin, histogram.ymin, histogram.xmax, histogram.ymax);

                for (x = 0; x < width; x++)
                {
                    int pixel;

                    // update ymin, ymax when adapt_radius changed (FILTER_ADAPTIVE)
                    ymin = std::max(0, y - adapt_radius);
                    ymax = std::min(height - 1, y + adapt_radius);
                    xmin = std::max(0, x - adapt_radius);
                    xmax = std::min(width - 1, x + adapt_radius);

                    histogram.update_histogram(img, cfg.black_level, cfg.white_level, width, xmin, ymin, xmax, ymax);

                    int pos = (x + (y * width));
                    pixel = histogram.get_median(pos);

                    if (cfg.filter_type == AdaptiveDespeckleConfig::RECURSIVE)
                    {
                        histogram.del_val(img, cfg.black_level, cfg.white_level, width, x, y);
                        for (int c = 0; c < img.channels(); c++)
                            img.set(c, pos, img.get(c, pixel));
                        histogram.add_val(img, cfg.black_level, cfg.white_level, width, x, y);
                    }

                    for (int c = 0; c < img.channels(); c++)
                        dsti.set(c, pos, img.get(c, pixel));

                    // Check the histogram and adjust the diameter accordingly...
                    if (cfg.filter_type == AdaptiveDespeckleConfig::ADAPTIVE)
                    {
                        if (histogram.hist0 >= adapt_radius || histogram.hist255 >= adapt_radius)
                        {
                            if (adapt_radius < cfg.radius)
                                adapt_radius++;
                        }
                        else if (adapt_radius > 1)
                        {
                            adapt_radius--;
                        }
                    }
                }

                if (progress != nullptr)
                    *progress = float(y) / float(height);
            }

            img = dsti;
        }
    } // namespace image
} // namespace satdump