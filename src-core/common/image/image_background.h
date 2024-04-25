#pragma once

#include "image.h"
#include "nlohmann/json.hpp"

namespace image
{
	template <typename T>
	void remove_background(image::Image<T> &img, nlohmann::json proj_cfg, float *progress);
}