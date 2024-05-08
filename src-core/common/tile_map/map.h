#pragma once

/*
#include "common/image/image.h"
#include "init.h"

#define TILE_SIZE 256
#define TILE_DL_LIMIT 250

struct mapTile
{
    int x, y;
    image::Image<uint8_t> data;

    mapTile(int x1, int y1, image::Image<uint8_t> tileImage);
};

struct tileMap
{
    std::string tileServerURL;
    std::string tileSaveDir;
    int expiryTime;

    tileMap(std::string url = "http://tile.openstreetmap.org/{z}/{x}/{y}.png", std::string path = satdump::user_path + "/osm_tiles/", int expiry = 30);
    image::Image<uint8_t> getMapImage(std::pair<float, float> coor, int zoom, std::pair<int, int> dim, float *progress);
    image::Image<uint8_t> getMapImage(std::pair<float, float> coor, std::pair<float, float> coor1, int zoom, float *progress);
    mapTile downloadTile(std::pair<int, int> t1, int zoom);
    std::pair<int, int> coorToTile(std::pair<float, float> coor, int zoom);
    std::pair<float, float> coorToTileF(std::pair<float, float> coor, int zoom);
    int widthToZoom(float deg, int width);
}; TODOIMG*/