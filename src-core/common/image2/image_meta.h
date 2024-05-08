#pragma once

#include "image.h"
#include "nlohmann/json.hpp"
#include "common/projection/reprojector.h"

namespace image2
{
    bool has_metadata(Image &img);
    void set_metadata(Image &img, nlohmann::json metadata);
    nlohmann::json get_metadata(const Image &img);
    void free_metadata(const Image &img);

    bool has_metadata_proj_cfg(Image &img);
    void set_metadata_proj_cfg(Image &img, nlohmann::json proj_cfg);
    nlohmann::json get_metadata_proj_cfg(const Image &img);
}