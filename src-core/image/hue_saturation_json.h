#pragma once

#include "image/hue_saturation.h"
#include "nlohmann/json.hpp"

namespace satdump
{
    namespace image
    {
        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(image::HueSaturation, hue, saturation, lightness, overlap)
    }
} // namespace satdump