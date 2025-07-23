#pragma once

/**
 * @file bowtie.h
 * @brief BowTie correction for multi-line instruments
 */

#include "image.h"
#include <vector>

namespace satdump
{
    namespace image
    {
        namespace bowtie
        {
            /**
             * @brief Performs instrument bowtie correction, required
             * for instruments such as MERSI, MODIS and such.
             *
             * @param inputImage the input image
             * @param channelCount number of channels in the image (inputImage.channels())
             * @param scanHeight the height of each scanline group
             * @param alpha value, representing the slant increase of the correction
             * @param beta value, being the offset of the correction
             * @param reverse_lut optionally, generates a LUT to get back the original image
             * pixel positions (eg, for calibration)
             * @return the corrected image
             */
            Image correctGenericBowTie(Image &inputImage, const int channelCount, const long scanHeight, const float alpha, const float beta, std::vector<std::vector<int>> *reverse_lut = nullptr);

            // template <typename T>
            // image::Image<T> correctSingleBowTie(image::Image<T> &inputImage, const int channelCount, const long scanHeight, const float alpha, const float beta);
        } // namespace bowtie
    } // namespace image
} // namespace satdump