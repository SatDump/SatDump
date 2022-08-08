#pragma once

#include "../app.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "common/image/image.h"

struct tile{
    int x, y;
    image::Image<uint8_t> data;

    tile(int x1, int y1, image::Image<uint8_t> tileImage);
};

struct tileMap{
    std::string tileServerURL;
    std::string tileSaveDir;
    int expiryTime;

    tileMap(std::string url = "http://tile.openstreetmap.org/", std::string path = "tiles/", int expiry = 30);
    image::Image<uint8_t> getMapImage(int zoom, float lat, float lon, int width, int height);
    image::Image<uint8_t> getMapImage(int zoom, float lat1, float lon1, float lat2, float lon2);
    tile downloadTile(std::pair<int, int> t1, int zoom);
    std::pair<int, int> coorToTile(float lat, float lon, int zoom);
};