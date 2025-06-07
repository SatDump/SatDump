#pragma once

// TODOREWORK THIS WHOLE FILE IS TEMPORARY
#include "image/image.h"
#include "nlohmann/json.hpp"

namespace satdump
{
    namespace projection
    {
        // Re-Projection operation
        struct ReprojectionOperation
        {
            image::Image *img;
            int output_width, output_height;
            nlohmann::json target_prj_info;
            bool use_old_algorithm = false;
        };

        image::Image reproject(ReprojectionOperation &op, float *progress = nullptr);
    } // namespace projection
} // namespace satdump