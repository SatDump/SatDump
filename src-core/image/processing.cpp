#include "image_utils.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <omp.h>
#include <thread>
#include "logger.h"
#include "utils/time.h"
#include "core/opencl.h"
#include "core/resources.h"

namespace satdump
{
    namespace image
    {
        int percentile(int *array, int size, float percentile)
        {
            float number_percent = (size + 1) * percentile / 100.0f;
            if (number_percent <= 1) // ==
                return array[0];
            else if (number_percent >= size) // ==
                return array[size - 1];
            else
                return array[(int)number_percent - 1] + (number_percent - (int)number_percent) * (array[(int)number_percent] - array[(int)number_percent - 1]);
        }

        inline int wraparound(int metric, int attempt)
        {
            if (attempt < 0)
                attempt += metric;
            if (metric <= attempt)
                attempt -= metric;

            return attempt;
        }

        void white_balance(Image &img, float percentileValue)
        {
            const float maxVal = img.maxval();
            const size_t d_height = img.height();
            const size_t d_width = img.width();

            int *sorted_array = new int[d_height * d_width];

            for (int c = 0; c < img.channels(); c++)
            {
                // Load the whole image band into our array
                for (size_t i = 0; i < d_height * d_width; i++)
                    sorted_array[i] = img.get(c, i);

                // Sort it
                std::sort(&sorted_array[0], &sorted_array[d_width * d_height]);

                // Get percentiles
                int percentile1 = percentile(sorted_array, d_width * d_height, percentileValue);
                int percentile2 = percentile(sorted_array, d_width * d_height, 100.0f - percentileValue);

                for (size_t i = 0; i < d_width * d_height; i++)
                {
                    long balanced = (img.get(c, i) - percentile1) * maxVal / (percentile2 - percentile1);
                    if (balanced < 0)
                        balanced = 0;
                    else if (balanced > maxVal)
                        balanced = maxVal;
                    img.set(c, i, balanced);
                }
            }

            delete[] sorted_array;
        }

        void median_blur(Image &img)
        {
            size_t h = img.height();
            size_t w = img.width();
            int num_threads = std::max(1, (int)(std::thread::hardware_concurrency() * 0.75));
            Image orig = img;

            for (int c = 0; c < img.channels(); c++)
            {
#pragma omp parallel for num_threads(num_threads)
                for (int64_t x = 0; x < (int64_t)h; x++)
                {
                    for (size_t y = 0; y < w; y++)
                    {
                        int values[5];
                        size_t idx = (size_t)x * w + y;
                        values[0] = values[1] = values[2] = values[3] = values[4] = orig.get(c, idx);

                        if (x != 0) values[1] = orig.get(c, (size_t)(x - 1) * w + y);
                        if (y != 0) values[2] = orig.get(c, (size_t)x * w + (y - 1));
                        if (x != (int64_t)h - 1) values[3] = orig.get(c, (size_t)(x + 1) * w + y);
                        if (y != w - 1) values[4] = orig.get(c, (size_t)x * w + (y + 1));

                        std::nth_element(values, values + 2, values + 5);
                        img.set(c, idx, values[2]);
                    }
                }
            }
        }

        void kuwahara_filter(Image &img)
        {
            const int radius = 1;
            const float num_pixels = (float)((radius + 1) * (radius + 1));
            const int d_channels = img.channels();
            const size_t d_width = img.width();
            const size_t d_height = img.height();
            int num_threads = std::max(1, (int)(std::thread::hardware_concurrency() * 0.75));
            Image orig = img;

            for (int c = 0; c < d_channels; c++)
            {
#pragma omp parallel for num_threads(num_threads)
                for (int64_t y = 0; y < (int64_t)d_height; y++)
                {
                    for (size_t x = 0; x < d_width; x++)
                    {
                        float average[4] = {0};
                        float variance[4] = {0};

                        // Calculate values for the four regions
                        for (int j = -radius; j <= 0; ++j)
                            for (int i = -radius; i <= 0; ++i)
                                average[0] += orig.get(c, wraparound(d_width, x + i), wraparound(d_height, y + j));
                        average[0] /= num_pixels;
                        for (int j = -radius; j <= 0; ++j)
                            for (int i = -radius; i <= 0; ++i)
                                variance[0] += pow(orig.get(c, wraparound(d_width, x + i), wraparound(d_height, y + j)) - average[0], 2);

                        for (int j = -radius; j <= 0; ++j)
                            for (int i = 0; i <= radius; ++i)
                                average[1] += orig.get(c, wraparound(d_width, x + i), wraparound(d_height, y + j));
                        average[1] /= num_pixels;
                        for (int j = -radius; j <= 0; ++j)
                            for (int i = 0; i <= radius; ++i)
                                variance[1] += pow(orig.get(c, wraparound(d_width, x + i), wraparound(d_height, y + j)) - average[1], 2);

                        for (int j = 0; j <= radius; ++j)
                            for (int i = 0; i <= radius; ++i)
                                average[2] += orig.get(c, wraparound(d_width, x + i), wraparound(d_height, y + j));
                        average[2] /= num_pixels;
                        for (int j = 0; j <= radius; ++j)
                            for (int i = 0; i <= radius; ++i)
                                variance[2] += pow(orig.get(c, wraparound(d_width, x + i), wraparound(d_height, y + j)) - average[2], 2);

                        for (int j = 0; j <= radius; ++j)
                            for (int i = -radius; i <= 0; ++i)
                                average[3] += orig.get(c, wraparound(d_width, x + i), wraparound(d_height, y + j));
                        average[3] /= num_pixels;
                        for (int j = 0; j <= radius; ++j)
                            for (int i = -radius; i <= 0; ++i)
                                variance[3] += pow(orig.get(c, wraparound(d_width, x + i), wraparound(d_height, y + j)) - average[3], 2);

                        // Find the region with the smallest variance and use its mean as the new pixel value
                        float min_sigma2 = FLT_MAX;
                        for (int k = 0; k < 4; k++)
                        {
                            variance[k] /= num_pixels - 1;
                            if (variance[k] < 0) variance[k] = -variance[k];

                            if (variance[k] < min_sigma2)
                            {
                                min_sigma2 = variance[k];
                                img.set(c, x, y, (int)(average[k] + 0.5f));
                            }
                        }
                    }
                }
            }
        }

        void equalize(Image &img, bool per_channel)
        {
            for (int c = 0; c < (per_channel ? img.channels() : 1); c++)
            {
                if (c == 3) // Do not individual equalize alpha channel
                    break;

                const int nlevels = img.maxval() + 1;
                size_t size = img.width() * img.height() * (per_channel ? 1 : img.channels());

                // Init histogram buffer
                int *histogram = new int[nlevels];
                for (int i = 0; i < nlevels; i++)
                    histogram[i] = 0;

                // Compute histogram
                for (size_t px = 0; px < size; px++)
                    histogram[img.get(c, px)]++;

                // Cummulative histogram
                int *cummulative_histogram = new int[nlevels];
                cummulative_histogram[0] = histogram[0];
                for (int i = 1; i < nlevels; i++)
                    cummulative_histogram[i] = histogram[i] + cummulative_histogram[i - 1];

                // Scaling
                int *scaling = new int[nlevels];
                for (int i = 0; i < nlevels; i++)
                    scaling[i] = round(cummulative_histogram[i] * (float(nlevels - 1) / size));

                // Apply
                for (size_t px = 0; px < size; px++)
                    img.set(c, px, img.clamp(scaling[img.get(c, px)]));

                // Cleanup
                delete[] cummulative_histogram;
                delete[] scaling;
                delete[] histogram;
            }
        }

        void normalize(Image &img)
        {
            int max = 0;
            int min = img.maxval();

            // Get min and max
            for (size_t i = 0; i < img.size(); i++)
            {
                int val = img.get(i);

                if (val > max)
                    max = val;
                if (val < min)
                    min = val;
            }

            if (abs(max - min) == 0) // Avoid division by 0
                return;

            // Compute scaling factor
            float factor = img.maxval() / float(max - min);

            // Scale entire image
            for (size_t i = 0; i < img.size(); i++)
                img.set(i, img.clamp((img.get(i) - min) * factor));
        }

        void linear_invert(Image &img)
        {
            for (size_t i = 0; i < img.size(); i++)
                img.set(i, img.maxval() - img.get(i));
        }

        void simple_despeckle(Image &img, int thresold)
        {
            size_t h = img.height();
            size_t w = img.width();
            int num_threads = std::max(1, (int)(std::thread::hardware_concurrency() * 0.75));
            Image orig = img;

            for (int c = 0; c < img.channels(); c++)
            {
#pragma omp parallel for num_threads(num_threads)
                for (int64_t x = 0; x < (int64_t)h; x++)
                {
                    for (size_t y = 0; y < w; y++)
                    {
                        unsigned short current = orig.get(c, (size_t)x * w + y);
                        unsigned short below = x + 1 == (int64_t)h ? 0 : orig.get(c, (size_t)(x + 1) * w + y);
                        unsigned short left = y - 1 == -1 ? 0 : orig.get(c, (size_t)x * w + (y - 1));
                        unsigned short right = y + 1 == w ? 0 : orig.get(c, (size_t)x * w + (y + 1));

                        if ((current - left > thresold && current - right > thresold) || (current - below > thresold && current - right > thresold))
                        {
                            img.set(c, (size_t)x * w + y, (right + left) / 2);
                        }
                    }
                }
            }
        }

        void switching_median(Image &img, int threshold)
        {
            Image orig = img;
            size_t h = img.height();
            size_t w = img.width();
            int num_threads = std::max(1, (int)(std::thread::hardware_concurrency() * 0.75));

            for (int c = 0; c < img.channels(); c++)
            {
#pragma omp parallel for num_threads(num_threads)
                for (int64_t x = 0; x < (int64_t)h; x++)
                {
                    for (size_t y = 0; y < w; y++)
                    {
                        int values[5];
                        size_t idx = (size_t)x * w + y;
                        values[0] = values[1] = values[2] = values[3] = values[4] = orig.get(c, idx);

                        if (x != 0) values[1] = orig.get(c, (size_t)(x - 1) * w + y);
                        if (y != 0) values[2] = orig.get(c, (size_t)x * w + (y - 1));
                        if (x != (int64_t)h - 1) values[3] = orig.get(c, (size_t)(x + 1) * w + y);
                        if (y != w - 1) values[4] = orig.get(c, (size_t)x * w + (y + 1));

                        std::nth_element(values, values + 2, values + 5);

                        int current = orig.get(c, idx);
                        int median = values[2];

                        if (abs(current - median) > threshold)
                            img.set(c, idx, median);
                    }
                }
            }
        }

        void adaptive_median(Image &img, int threshold_strong)
        {
            Image orig = img;
            size_t h = img.height();
            size_t w = img.width();
            int num_threads = std::max(1, (int)(std::thread::hardware_concurrency() * 0.75));

            for (int c = 0; c < img.channels(); c++)
            {
#pragma omp parallel for num_threads(num_threads)
                for (int64_t x = 0; x < (int64_t)h; x++)
                {
                    for (size_t y = 0; y < w; y++)
                    {
                        int v3[9];
                        int count3 = 0;
                        for (int i = -1; i <= 1; i++)
                            for (int j = -1; j <= 1; j++)
                                if (x + i >= 0 && x + i < (int64_t)h && y + j >= 0 && y + j < w)
                                    v3[count3++] = orig.get(c, (size_t)(x + i) * w + (y + j));

                        std::nth_element(v3, v3 + count3 / 2, v3 + count3);
                        int m3 = v3[count3 / 2];
                        int current = orig.get(c, (size_t)x * w + y);

                        if (abs(current - m3) > threshold_strong)
                        {
                            int v5[25];
                            int count5 = 0;
                            for (int i = -2; i <= 2; i++)
                                for (int j = -2; j <= 2; j++)
                                    if (x + i >= 0 && x + i < (int64_t)h && y + j >= 0 && y + j < w)
                                        v5[count5++] = orig.get(c, (size_t)(x + i) * w + (y + j));

                            std::nth_element(v5, v5 + count5 / 2, v5 + count5);
                            img.set(c, (size_t)x * w + y, v5[count5 / 2]);
                        }
                        else
                        {
                            img.set(c, (size_t)x * w + y, m3);
                        }
                    }
                }
            }
        }

        void bilateral_filter(Image &img, int radius, float sigma_intensity)
        {
            Image orig = img;
            size_t h = img.height();
            size_t w = img.width();
            int num_threads = std::max(1, (int)(std::thread::hardware_concurrency() * 0.75));
            float sigma_space = radius / 2.0f;

            // Automatically scale sigma_intensity to the image dynamic range
            // (Assuming sigma_intensity 25 is "medium" for 8-bit images)
            float scaled_sigma = sigma_intensity * (img.maxval() / 255.0f);
            if (scaled_sigma < 0.1f) scaled_sigma = 0.1f;

            // Precalculate spatial weights
            std::vector<float> spatial_weights((2 * radius + 1) * (2 * radius + 1));
            for (int i = -radius; i <= radius; i++)
                for (int j = -radius; j <= radius; j++)
                    spatial_weights[(i + radius) * (2 * radius + 1) + (j + radius)] = exp(-(float)(i * i + j * j) / (2 * sigma_space * sigma_space));

            // Intensity difference LUT (Up to 16-bit range)
            std::vector<float> range_lut(65536);
            double inv_sigma_range = 1.0 / (2.0 * (double)scaled_sigma * (double)scaled_sigma);
            for (int i = 0; i < 65536; i++)
                range_lut[i] = exp(-((double)i * (double)i) * inv_sigma_range);

            for (int c = 0; c < img.channels(); c++)
            {
#pragma omp parallel for num_threads(num_threads)
                for (int64_t x = 0; x < (int64_t)h; x++)
                {
                    for (size_t y = 0; y < w; y++)
                    {
                        float sum_weights = 0;
                        float sum_values = 0;
                        int current = orig.get(c, (size_t)x * w + y);

                        for (int i = -radius; i <= radius; i++)
                        {
                            for (int j = -radius; j <= radius; j++)
                            {
                                int64_t nx = x + i;
                                int64_t ny = (int64_t)y + j;
                                if (nx >= 0 && nx < (int64_t)h && ny >= 0 && ny < (int64_t)w)
                                {
                                    int neighbor = orig.get(c, (size_t)nx * w + (size_t)ny);
                                    int diff = abs(current - neighbor);
                                    
                                    float w_space = spatial_weights[(i + radius) * (2 * radius + 1) + (j + radius)];
                                    float w_range = (diff < 65536) ? range_lut[diff] : 0.0f;

                                    float weight = w_space * w_range;
                                    sum_weights += weight;
                                    sum_values += neighbor * weight;
                                }
                            }
                        }
                        if (sum_weights > 0.00001f)
                            img.set(c, (size_t)x * w + y, (int)(sum_values / sum_weights + 0.5f));
                    }
                }
            }
        }

        void simple_inpainting(Image &img, int threshold)
        {
            Image orig = img;
            size_t h = img.height();
            size_t w = img.width();
            int num_threads = std::max(1, (int)(std::thread::hardware_concurrency() * 0.75));

            for (int c = 0; c < img.channels(); c++)
            {
#pragma omp parallel for num_threads(num_threads)
                for (int64_t x = 0; x < (int64_t)h; x++)
                {
                    for (size_t y = 0; y < w; y++)
                    {
                        int v3[9];
                        int count3 = 0;
                        for (int i = -1; i <= 1; i++)
                            for (int j = -1; j <= 1; j++)
                                if (x + i >= 0 && x + i < (int64_t)h && y + j >= 0 && y + j < w)
                                    v3[count3++] = orig.get(c, (size_t)(x + i) * w + (y + j));

                        std::nth_element(v3, v3 + count3 / 2, v3 + count3);
                        int m3 = v3[count3 / 2];
                        int current = orig.get(c, (size_t)x * w + y);

                        if (abs(current - m3) > threshold)
                        {
                            float sum = 0;
                            int count = 0;
                            for (int i = -1; i <= 1; i++)
                            {
                                for (int j = -1; j <= 1; j++)
                                {
                                    if (i == 0 && j == 0) continue;
                                    int64_t nx = x + i;
                                    int64_t ny = (int64_t)y + j;
                                    if (nx >= 0 && nx < (int64_t)h && ny >= 0 && ny < (int64_t)w)
                                    {
                                        int val = orig.get(c, (size_t)nx * w + (size_t)ny);
                                        if (abs(val - m3) <= threshold)
                                        {
                                            sum += val;
                                            count++;
                                        }
                                    }
                                }
                            }
                            if (count > 0)
                                img.set(c, (size_t)x * w + y, (int)(sum / count + 0.5f));
                            else
                                img.set(c, (size_t)x * w + y, m3);
                        }
                        else
                        {
                            img.set(c, (size_t)x * w + y, current);
                        }
                    }
                }
            }
        }

        void selective_impulse_filter(Image &img, int threshold, int window_size)
        {
            Image orig = img;
            size_t h = img.height();
            size_t w = img.width();
            int num_threads = std::max(1, (int)(std::thread::hardware_concurrency() * 0.75));
            int r = (window_size - 1) / 2;

            for (int c = 0; c < img.channels(); c++)
            {
#pragma omp parallel for num_threads(num_threads)
                for (int64_t x = 0; x < (int64_t)h; x++)
                {
                    std::vector<int> values;
                    values.reserve(window_size * window_size);

                    for (size_t y = 0; y < w; y++)
                    {
                        values.clear();
                        int current = orig.get(c, (size_t)x * w + y);
                        
                        for (int i = -r; i <= r; i++)
                        {
                            for (int j = -r; j <= r; j++)
                            {
                                int64_t nx = x + i;
                                int64_t ny = (int64_t)y + j;
                                if (nx >= 0 && nx < (int64_t)h && ny >= 0 && ny < (int64_t)w)
                                {
                                    values.push_back(orig.get(c, (size_t)nx * w + (size_t)ny));
                                }
                            }
                        }

                        // We need the entire sorted window to find gaps
                        std::sort(values.begin(), values.end());
                        int median = values[values.size() / 2];

                        // A pixel is noise ONLY if it's strictly an isolated outlier
                        bool updated = false;

                        // Check if current is the absolute minimum AND there's a big gap to the next value
                        if (current == values[0])
                        {
                            // If it's a lone black pixel, the NEXT value in sorted list will be much higher
                            int next_nearest = values[1];
                            if ((next_nearest - current) > threshold)
                            {
                                img.set(c, (size_t)x * w + y, median);
                                updated = true;
                            }
                        }
                        
                        // Check if current is the absolute maximum AND there's a big gap to the previous value
                        if (!updated && current == values[values.size() - 1])
                        {
                            // If it's a lone white pixel, the PREVIOUS value in sorted list will be much lower
                            int prev_nearest = values[values.size() - 2];
                            if ((current - prev_nearest) > threshold)
                            {
                                img.set(c, (size_t)x * w + y, median);
                            }
                        }
                    }
                }
            }
        }


        void scanline_noise_remover(Image &img, int threshold, int radius)
        {
            double t0 = getTime();
            Image orig = img;
            size_t h = img.height();
            size_t w = img.width();
            int channels = img.channels();
            int num_threads = std::max(1, (int)(std::thread::hardware_concurrency() * 0.75));
            double sigma_multiplier = threshold / 10.0;

            for (int c = 0; c < channels; c++)
            {
#pragma omp parallel for num_threads(num_threads)
                for (int64_t y = 0; y < (int64_t)h; y++)
                {
                    double sum = 0;
                    double sum_sq = 0;
                    int count = 0;

                    // Helper to get pixel with border clamping
                    auto get_pix = [&](int64_t ix) {
                        if (ix < 0) ix = 0;
                        if (ix >= (int64_t)w) ix = w - 1;
                        return (double)orig.get(c, (size_t)ix, (size_t)y);
                    };

                    // Initial window setup for the start of the scanline
                    for (int i = -radius; i <= radius; i++)
                    {
                        double val = get_pix(i);
                        sum += val;
                        sum_sq += val * val;
                        count++;
                    }

                    for (size_t x = 0; x < w; x++)
                    {
                        double mean = sum / count;
                        double variance = (sum_sq / count) - (mean * mean);
                        if (variance < 0) variance = 0;
                        double std_dev = sqrt(variance);
                        if (std_dev < 1.0) std_dev = 1.0;

                        int current = (int)get_pix(x); 
                        if (abs(current - mean) > sigma_multiplier * std_dev)
                        {
                            int previous = (x > 0) ? (int)get_pix(x - 1) : (int)mean;
                            img.set(c, x, y, previous);
                        }

                        // Sliding window update
                        double old_val = get_pix((int64_t)x - radius);
                        double new_val = get_pix((int64_t)x + radius + 1);
                        sum = sum - old_val + new_val;
                        sum_sq = sum_sq - (old_val * old_val) + (new_val * new_val);
                    }
                }
            }
            logger->info("Scanline noise removal took: %7.2f ms", (getTime() - t0) * 1000.0);
        }
    } // namespace image
} // namespace satdump