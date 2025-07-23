#pragma once

/**
 * @file  processing.h
 * @brief Various processing functions (blur, equalize...)
 */

#include "image.h"

namespace satdump
{
    namespace image
    {
        /**
         * @brief While balance algorithm from Gimp
         * @param img image to white balance
         * @param percentileValue percentile to balance to
         */
        void white_balance(Image &img, float percentileValue = 0.05f);

        /**
         * @brief Median blur. Somewhat equivalent to Gimp's with
         * a radius of 1
         * @param img image to blur
         */
        void median_blur(Image &img);

        /**
         * @brief Adaptive noise reduction filter
         * @param img image to process
         */
        void kuwahara_filter(Image &img);

        /**
         * @brief Perform histogram equalization
         * @param img image to equalize
         * @param per_channel if true, performs the equalization
         * separately for each spectral channel
         */
        void equalize(Image &img, bool per_channel = false);

        /**
         * @brief Normalize the current image data. This takes
         * the current min and max values and extends it to the full bit
         * depth range
         * @param img image to process
         */
        void normalize(Image &img);

        /**
         * @brief Invert the entire image. Blacks becomes white and the
         * opposite
         * @param img image to process
         */
        void linear_invert(Image &img);

        /**
         * @brief Despeckle the image. Very basic despeckle algorithm
         * @param img image to process
         * @param thresold difference over which the pixel is
         * considered bad
         */
        void simple_despeckle(Image &img, int thresold = 10);
    } // namespace image
} // namespace satdump