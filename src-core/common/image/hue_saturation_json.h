#pragma once

#include "common/image/hue_saturation.h"
#include "nlohmann/json.hpp"

namespace image
{
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(image::HueSaturation, hue, saturation, lightness, overlap)
}