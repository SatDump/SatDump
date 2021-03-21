#include "image.h"

namespace image
{
    int percentile(unsigned short *array, int size, float percentile)
    {
        float number_percent = (size + 1) * percentile / 100.0f;
        if (number_percent == 1)
            return array[0];
        else if (number_percent == size)
            return array[size - 1];
        else
            return array[(int)number_percent - 1] + (number_percent - (int)number_percent) * (array[(int)number_percent] - array[(int)number_percent - 1]);
    }

    void white_balance(cimg_library::CImg<unsigned short> &image, float percentileValue, int channelCount)
    {
        int height = image.height();
        int width = image.width();

        unsigned short *sorted_array = new unsigned short[height * width];

        for (int band_number = 0; band_number < channelCount; band_number++)
        {
            // Load the whole image band into our array
            std::memcpy(sorted_array, &image.data()[band_number * width * height], width * height * sizeof(unsigned short));

            // Sort it
            std::sort(&sorted_array[0], &sorted_array[width * height]);

            // Get percentiles
            int percentile1 = percentile(sorted_array, width * height, percentileValue);
            int percentile2 = percentile(sorted_array, width * height, 100.0f - percentileValue);

            for (int i = 0; i < width * height; i++)
            {
                long balanced = (image[band_number * width * height + i] - percentile1) * 65535.0f / (percentile2 - percentile1);
                if (balanced < 0)
                    balanced = 0;
                else if (balanced > 65535)
                    balanced = 65535;
                image[band_number * width * height + i] = balanced;
            }
        }
    }
}