#pragma once

#include <string>
#include "image/image.h"

image::Image downloadTileMap(std::string url_source, double lat0, double lon0, double lat1, double lon1, int zoom);
