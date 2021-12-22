#pragma once

#include <cstdint>
#include "image.h"

namespace image
{
    // Forward FFT
    void fft_forward(Image<uint16_t> &image);

    // Inverse FFT
    void fft_inverse(Image<uint16_t> &image);
}