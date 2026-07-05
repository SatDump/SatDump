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

        /**
         * @brief Switching Median filter. Only replaces pixels
         * considered bad (noise spikes) with the median.
         * @param img image to process
         * @param thresold difference over which the pixel is
         * considered bad
         */
        void switching_median(Image &img, int thresold = 40);

        /**
         * @brief Adaptive (Cascaded) Median filter. Uses 3x3 or 5x5
         * median depending on the noise level.
         * @param img image to process
         * @param threshold_strong difference over which the 5x5 median
         * is preferred over 3x3
         */
        void adaptive_median(Image &img, int threshold_strong = 60);

        /**
         * @brief Bilateral Filter. Edge-preserving smoother.
         * @param img image to process
         * @param radius spatial radius
         * @param sigma_intensity intensity difference sigma
         */
        void bilateral_filter(Image &img, int radius = 3, float sigma_intensity = 25.0f);

        /**
         * @brief Simple Inpainting (Noise filling).
         * @param img image to process
         * @param threshold difference over which the pixel is
         * considered "bad" and needs to be filled
         */
        void simple_inpainting(Image &img, int threshold = 40);

        /**
         * @brief Selective Impulse Filter. Only replaces pixel if it's
         * local min/max and exceeds threshold from median.
         * @param img image to process
         * @param threshold difference from median
         * @param window_size size of the search window (3, 5, 7...)
         */
        void selective_impulse_filter(Image &img, int threshold = 50, int window_size = 3);

        /**
         * @brief Scanline Noise Remover (1D Horizontal Impulse filter).
         * Optimized CPU implementation (Sliding window).
         * @param img image to process
         * @param threshold difference from local horizontal mean
         * @param radius radius of the search window (1D)
         */
        void scanline_noise_remover(Image &img, int threshold = 40, int radius = 5);
    } // namespace image
} // namespace satdump