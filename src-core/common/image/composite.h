#pragma once

#include <cstdint>
#include "image.h"
#include <string>
#include "nlohmann/json.hpp"

namespace image
{
    // Generate a composite from channels and an equation
    template <typename T>
    Image<T> generate_composite_from_equ(std::vector<Image<T>> inputChannels, std::vector<int> channelNumbers, std::string equation, nlohmann::json parameters);
}