#include "image.h"
#include <cstring>
#include <cmath>
#include <limits>
#include <algorithm>

#include "logger.h"

namespace image
{
    template <typename T>
    void Image<T>::fill_color(T color[])
    {
        for (int c = 0; c < d_channels; c++)
            for (int i = 0; i < d_width * d_height; i++)
                channel(c)[i] = color[c];
    }

    template <typename T>
    void Image<T>::fill(T val)
    {
        for (int c = 0; c < d_channels; c++)
            for (int i = 0; i < d_width * d_height; i++)
                channel(c)[i] = val;
    }

    template <typename T>
    void Image<T>::mirror(bool x, bool y)
    {
        if (y) // Mirror on the Y axis
        {
            T *tmp_col = new T[d_height];

            for (int c = 0; c < d_channels; c++)
            {
                for (int col = 0; col < d_width; col++)
                {
                    for (int i = 0; i < d_height; i++) // Buffer column
                        tmp_col[i] = channel(c)[i * d_width + col];

                    for (int i = 0; i < d_height; i++) // Restore and mirror
                        channel(c)[i * d_width + col] = tmp_col[(d_height - 1) - i];
                }
            }

            delete[] tmp_col;
        }

        if (x) // Mirror on the X axis
        {
            T *tmp_row = new T[d_width];

            for (int c = 0; c < d_channels; c++)
            {
                for (int row = 0; row < d_height; row++)
                {
                    for (int i = 0; i < d_width; i++) // Buffer column
                        tmp_row[i] = channel(c)[row * d_width + i];

                    for (int i = 0; i < d_width; i++) // Restore and mirror
                        channel(c)[row * d_width + i] = tmp_row[(d_width - 1) - i];
                }
            }

            delete[] tmp_row;
        }
    }

    template <typename T>
    Image<T> &Image<T>::equalize()
    {
        for (int c = 0; c < d_channels; c++)
        {
            T *data_ptr = channel(c);

            int nlevels = std::numeric_limits<T>::max() + 1;
            int size = d_width * d_height;

            // Init histogram buffer
            int *histogram = new int[nlevels];
            for (int i = 0; i < nlevels; i++)
                histogram[i] = 0;

            // Compute histogram
            for (int px = 0; px < size; px++)
                histogram[data_ptr[px]]++;

            // Cummulative histogram
            int *cummulative_histogram = new int[nlevels];
            cummulative_histogram[0] = histogram[0];
            for (int i = 1; i < nlevels; i++)
                cummulative_histogram[i] = histogram[i] + cummulative_histogram[i - 1];

            // Scaling
            int *scaling = new int[nlevels];
            for (int i = 0; i < nlevels; i++)
                scaling[i] = round(cummulative_histogram[i] * (float(std::numeric_limits<T>::max()) / size));

            // Apply
            for (int px = 0; px < size; px++)
                data_ptr[px] = clamp(scaling[data_ptr[px]]);

            // Cleanup
            delete[] cummulative_histogram;
            delete[] scaling;
            delete[] histogram;
        }

        return *this;
    }

    template <typename T>
    Image<T> &Image<T>::normalize()
    {
        int max = 0;
        int min = std::numeric_limits<T>::max();

        // Get min and max
        for (size_t i = 0; i < data_size; i++)
        {
            int val = d_data[i];

            if (val > max)
                max = val;
            if (val < min)
                min = val;
        }

        if (abs(max - min) == 0) // Avoid division by 0
            return *this;

        // Compute scaling factor
        int factor = std::numeric_limits<T>::max() / (max - min);

        // Scale entire image
        for (size_t i = 0; i < data_size; i++)
            d_data[i] = clamp((d_data[i] - min) * factor);

        return *this;
    }

    template <typename T>
    void Image<T>::crop(int x0, int y0, int x1, int y1)
    {
        int new_width = x1 - x0;
        int new_height = y1 - y0;

        // Create new buffer
        T *new_data = new T[new_width * new_height * d_channels];

        // Copy cropped area to new region
        for (int c = 0; c < d_channels; c++)
            for (int x = 0; x < new_width; x++)
                for (int y = 0; y < new_height; y++)
                    new_data[(new_width * new_height * c) + y * new_width + x] = channel(c)[(y0 + y) * d_width + (x + x0)];

        // Swap out buffer
        delete[] d_data;
        d_data = new_data;

        // Update info
        data_size = new_width * new_height * d_channels;
        d_width = new_width;
        d_height = new_height;
    }

    template <typename T>
    void Image<T>::crop(int x0, int x1)
    {
        crop(x0, 0, x1, d_height);
    }

    template <typename T>
    void Image<T>::resize(int width, int height)
    {
        double x_scale = double(d_width) / double(width);
        double y_scale = double(d_height) / double(height);

        Image<T> tmp = *this;
        init(width, height, d_channels);

        for (int c = 0; c < d_channels; c++)
        {
            for (int x = 0; x < d_width; x++)
            {
                for (int y = 0; y < d_height; y++)
                {
                    int xx = floor(double(x) * x_scale);
                    int yy = floor(double(y) * y_scale);

                    channel(c)[y * d_width + x] = tmp.channel(c)[yy * tmp.width() + xx];
                }
            }
        }
    }

    template <typename T>
    void Image<T>::resize_bilinear(int width, int height, bool text_mode)
    {
        int a, b, c, d, x, y, index;
        double x_scale = double(d_width - 1) / double(width);
        double y_scale = double(d_height - 1) / double(height);
        float x_diff, y_diff, val;

        Image<T> tmp = *this;
        init(width, height, d_channels);

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

                    a = tmp.channel(cc)[index];
                    b = tmp.channel(cc)[index + 1];
                    c = tmp.channel(cc)[index + tmp.width()];
                    d = tmp.channel(cc)[index + tmp.width() + 1];

                    val = a * (1 - x_diff) * (1 - y_diff) +
                          b * (x_diff) * (1 - y_diff) +
                          c * (y_diff) * (1 - x_diff) +
                          d * (x_diff * y_diff);

                    if (text_mode) // Special text mode, where we want to keep it clear whatever the res is
                        channel(cc)[i * width + j] = val > 0 ? std::numeric_limits<T>::max() : 0;
                    else
                        channel(cc)[i * width + j] = val;
                }
            }
        }
    }

    template <typename T>
    int percentile(T *array, int size, float percentile)
    {
        float number_percent = (size + 1) * percentile / 100.0f;
        if (number_percent == 1)
            return array[0];
        else if (number_percent == size)
            return array[size - 1];
        else
            return array[(int)number_percent - 1] + (number_percent - (int)number_percent) * (array[(int)number_percent] - array[(int)number_percent - 1]);
    }

    template <typename T>
    void Image<T>::white_balance(float percentileValue)
    {
        float maxVal = std::numeric_limits<T>::max();

        T *sorted_array = new T[d_height * d_width];

        for (int c = 0; c < d_channels; c++)
        {
            // Load the whole image band into our array
            std::memcpy(sorted_array, channel(c), d_width * d_height * sizeof(T));

            // Sort it
            std::sort(&sorted_array[0], &sorted_array[d_width * d_height]);

            // Get percentiles
            int percentile1 = percentile(sorted_array, d_width * d_height, percentileValue);
            int percentile2 = percentile(sorted_array, d_width * d_height, 100.0f - percentileValue);

            for (int i = 0; i < d_width * d_height; i++)
            {
                long balanced = (channel(c)[i] - percentile1) * maxVal / (percentile2 - percentile1);
                if (balanced < 0)
                    balanced = 0;
                else if (balanced > maxVal)
                    balanced = maxVal;
                channel(c)[i] = balanced;
            }
        }

        delete[] sorted_array;
    }

    template <typename T>
    void Image<T>::brightness_contrast_old(float brightness, float contrast)
    {
        float brightness_v = brightness / 2.0f;
        float slant = tanf((contrast + 1.0f) * 0.78539816339744830961566084581987572104929234984378f);
        const float max = std::numeric_limits<T>::max();

        for (size_t i = 0; i < data_size; i++)
        {
            float v = d_data[i];

            if (brightness_v < 0.0f)
                v = v * (max + brightness_v);
            else
                v = v + ((max - v) * brightness_v);

            v = (v - (max / 2)) * slant + (max / 2);

            d_data[i] = clamp(v * 2.0f);
        }
    }

    template <typename T>
    void Image<T>::linear_invert()
    {
        for (size_t i = 0; i < data_size; i++)
            d_data[i] = std::numeric_limits<T>::max() - d_data[i];
    }

    template <typename T>
    void Image<T>::simple_despeckle(int thresold)
    {
        for (int c = 0; c < d_channels; c++)
        {
            T *data_ptr = channel(c);

            int h = d_height;
            int w = d_width;

            for (int x = 0; x < h; x++)
            {
                for (int y = 0; y < w; y++)
                {
                    unsigned short current = data_ptr[x * w + y];

                    unsigned short below = x + 1 == h ? 0 : data_ptr[(x + 1) * w + y];
                    unsigned short left = y - 1 == -1 ? 0 : data_ptr[x * w + (y - 1)];
                    unsigned short right = y + 1 == w ? 0 : data_ptr[x * w + (y + 1)];

                    if ((current - left > thresold && current - right > thresold) ||
                        (current - below > thresold && current - right > thresold))
                    {
                        data_ptr[x * w + y] = (right + left) / 2;
                    }
                }
            }
        }
    }

    // Generate Images for uint16_t and uint8_t
    template class Image<uint8_t>;
    template class Image<uint16_t>;
}