#pragma once

#include "image.h"
#include "nlohmann/json.hpp"

namespace image
{
	void remove_background(Image &img, nlohmann::json proj_cfg, float *progress);
}