#pragma once

#include <cstdint>
#include "image.h"
#include <string>
#include "nlohmann/json.hpp"
#include "products/image_products.h"

namespace image
{
    // Generate a composite from channels and an equation
    template <typename T>
    Image<T> generate_composite_from_equ(std::vector<Image<T>> inputChannels, std::vector<std::string> channelNumbers, std::string equation, nlohmann::json offsets_cfg, float *progress = nullptr);

    // Generate a composite from channels and a LUT
    template <typename T>
    Image<T> generate_composite_from_lut(std::vector<Image<T>> inputChannels, std::vector<std::string> channelNumbers, std::string lut_path, nlohmann::json offsets_cfg, float *progress = nullptr);

    // Generate a composite from channels and a Lua script
    template <typename T>
    Image<T> generate_composite_from_lua(satdump::ImageProducts *img_pro, std::vector<Image<T>> inputChannels, std::vector<std::string> channelNumbers, std::string lua_path, nlohmann::json lua_vars, nlohmann::json offsets_cfg, float *progress = nullptr);
}