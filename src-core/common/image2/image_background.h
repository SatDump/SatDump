#pragma once

#include "image.h"
#include "nlohmann/json.hpp"

namespace image2
{
	void remove_background(Image &img, nlohmann::json proj_cfg, float *progress);
}