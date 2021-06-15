#pragma once

#include <cstdint>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace image
{
    // Forward FFT
    void fft_forward(cimg_library::CImg<unsigned short> &image);

    // Inverse FFT
    void fft_inverse(cimg_library::CImg<unsigned short> &image);
}