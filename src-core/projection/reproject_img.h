#pragma once

/**
 * @file reproject_img.h
 */

#include "image/image.h"
#include "nlohmann/json.hpp"

namespace satdump
{
    namespace projection
    {
        /**
         * @brief Reproject an image to another projection
         * @param input image to reproject
         * @param target_proj target projection
         * @param progress optional progress variable
         * @return reprojected image. 0 if failed
         */
        image::Image reprojectImage(image::Image &input, nlohmann::json target_prj, float *progress = nullptr);
    } // namespace projection
} // namespace satdump