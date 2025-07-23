#pragma once

/**
 * @file earth_curvature.h
 * @brief Earth curvature correction for simple instruments (eg, AVHRR)
 */

#include "image.h"

namespace satdump
{
    namespace image
    {
        namespace earth_curvature
        {
            /**
             * @brief This allows correcting the effect of the earth's
             * curvature and instrument scanning methods to generate an
             * image with a more linear relation between pixels and the
             * Earth's surface distances. It is meant to correct images
             * from nadir-pointing instruments scanning on the roll axis
             * at a constant angle between pixels.
             * @param image input image
             * @param satellite_height satellite orbit height (in km)
             * @param swath instrument swath in km
             * @param resolution_km instrument ground resolution in km
             * @param foward_table optional LUT generated. Input => Corrected
             * @param reverse_table optional LUT generated. Corrected => Input
             * @return corrected image
             */
            Image correct_earth_curvature(Image &image, float satellite_height, float swath, float resolution_km, std::vector<float> *foward_table = nullptr,
                                          std::vector<float> *reverse_table = nullptr);

            /**
             * @brief The same as correct_earth_curvature, but automatically
             * reading the correction's configuration in the image metadata.
             * @param img input image
             * @param success will be set to true on success
             * @param foward_table optional LUT generated. Input => Corrected
             * @param reverse_table optional LUT generated. Corrected => Input
             * @return corrected image
             */
            Image perform_geometric_correction(image::Image img, bool &success, std::vector<float> *foward_table = nullptr, std::vector<float> *reverse_table = nullptr);
        } // namespace earth_curvature
    } // namespace image
} // namespace satdump