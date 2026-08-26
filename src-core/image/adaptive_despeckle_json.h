#pragma once

#include "image/adaptive_despeckle.h"
#include "image/hue_saturation.h"
#include "nlohmann/json.hpp"

namespace satdump
{
    namespace image
    {
        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(image::AdaptiveDespeckleConfig, filter_type, radius, black_level, white_level)
    }
} // namespace satdump