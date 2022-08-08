#pragma once

#include "../app.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "common/image/image.h"

struct tileMap{
    tileMap(std::string url, std::string path = "./tiles/");
    std::string tileServerURL;
    std::string tileSaveDir;
    image::Image<uint8_t> getMapImage(int zoom, float lat, float lon, int width, int height);
    image::Image<uint8_t> getMapImage(int zoom, float lat1, float lon1, float lat2, float lon2);
    std::vector<std::vector<image::Image<uint8_t>>> assembleTiles(std::pair<int, int> t1, std::pair<int, int> t2);
    void downloadTile(std::pair<int, int> t1, int zoom);
    std::pair<int, int> coorToTile(float lat, float lon, int zoom);
};