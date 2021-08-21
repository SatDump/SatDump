#pragma once

#include <cstdint>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace image
{
    // Implementation of GIMP's White Balance algorithm
    void white_balance(cimg_library::CImg<unsigned short> &image, float percentileValue = 0.05f, int channelCount = 3);

    // Decompress JPEG Data from memory
    cimg_library::CImg<unsigned char> decompress_jpeg(uint8_t *data, int length, bool ignore_errors = false);

    // Simple despeckle
    void simple_despeckle(cimg_library::CImg<unsigned short> &image, int thresold);

    // Percentile application
    void extract_percentile(cimg_library::CImg<unsigned short> &image, float percentile1, float percentile2, int channelCount = 3);

    // Linear invert
    template <typename T>
    void linear_invert(cimg_library::CImg<T> &image);

    // Contrast and brightness correction
    void brightness_contrast_old(cimg_library::CImg<unsigned short> &image, float brightness, float contrast, int channelCount = 3);
}