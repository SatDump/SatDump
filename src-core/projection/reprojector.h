#pragma once

// TODOREWORK THIS WHOLE FILE IS TEMPORARY
#include "nlohmann/json.hpp"
#include "common/image/image.h"

namespace satdump
{
    namespace proj
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
    }
}