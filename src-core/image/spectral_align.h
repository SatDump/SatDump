#pragma once

/**
 * @file  spectral_align.h
 * @brief Correlate images of different spectral channels
 */

#include "image.h"

namespace satdump
{
    namespace image
    {
        /**
         * @brief Correlate 2 images (2D FFT) to align them. Both MUST be of the same resolution/bit depth, and one channel!
         * This can only handle a translation, no rotation or other deformations
         * @param img1 Reference image
         * @param img2 Image to correlate
         * @param x, y output offset
         * @return true on error, false on success
         */
        bool correlate_fft2d(image::Image &img1, image::Image &img2, double &x, double &y);
    } // namespace image
} // namespace satdump