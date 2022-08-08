#include "map.h"
#include <math.h>
#define _USE_MATH_DEFINES
#include "common/utils.h"
#include <filesystem>
#include <chrono>
#include <iostream>

tileMap::tileMap(std::string url, std::string path, int expiry)
{
    tileServerURL = url;
    tileSaveDir = path;
    expiryTime = expiry;
}

std::pair<int, int> tileMap::coorToTile(std::pair<float, float> coor, int zoom)
{
    int x, y;
    int n = std::pow(2, zoom);
    x = n * ((coor.second + 180) / 360);
    y = n * (1 - (log(tan(coor.first * (M_PI / 180)) + 1 / cos(coor.first * (M_PI / 180))) / M_PI)) / 2;
    return {x, y};
}

std::pair<float, float> tileMap::coorToTileF(std::pair<float, float> coor, int zoom)
{
    float x, y;
    float n = std::pow(2, zoom);
    x = n * ((coor.second + 180.0) / 360.0);
    y = n * (1.0 - (log(tan(coor.first * (M_PI / 180.0)) + 1 / cos(coor.first * (M_PI / 180.0))) / M_PI)) / 2.0;
    return {x, y};
}

mapTile tileMap::downloadTile(std::pair<int, int> t1, int zoom)
{
    image::Image<uint8_t> img;
    if (t1.first >= pow(2, zoom))
        t1.first -= ((t1.first / (int)pow(2, zoom)) * pow(2, zoom));
    if (t1.second >= pow(2, zoom))
    {
        img = image::Image<uint8_t>(TILE_SIZE, TILE_SIZE, 3);
        img.fill(120);
        return {t1.first, t1.second, img};
    }
    std::string filename = tileSaveDir + std::to_string(zoom) + "/" + std::to_string(t1.first) + "/" + std::to_string(t1.second) + ".png";
    bool old = false;
    if (std::filesystem::exists(filename))
    {
        std::filesystem::file_time_type ftime = std::filesystem::last_write_time(filename);
        int64_t t1 = std::chrono::duration_cast<std::chrono::hours>(ftime.time_since_epoch()).count();
        int64_t t2 = std::chrono::duration_cast<std::chrono::hours>(std::chrono::system_clock::now().time_since_epoch()).count();
        old = (t2 - t1) / 24 > expiryTime;
    }

    if (!std::filesystem::exists(filename) /*|| old*/)
    {
        std::string res;
        std::string url = tileServerURL + std::to_string(zoom) + "/" + std::to_string(t1.first) + "/" + std::to_string(t1.second) + ".png";
        perform_http_request(url, res);
        img.load_png((uint8_t *)res.data(), res.size());
        std::filesystem::create_directories(tileSaveDir + std::to_string(zoom) + "/" + std::to_string(t1.first) + "/");
        std::ofstream of(filename);
        of << res;
        of.close();
    }
    else
    {
        img.load_png(filename);
    }
    return mapTile(t1.first, t1.second, img);
}

mapTile::mapTile(int x1, int y1, image::Image<uint8_t> tileImage)
{
    x = x1;
    y = y1;
    data = tileImage;
}

image::Image<uint8_t> tileMap::getMapImage(std::pair<float, float> coor, int zoom, std::pair<int, int> dim)
{
    image::Image<uint8_t> img(dim.first, dim.second, 3);
    int xtiles = ceill(dim.first / (float)TILE_SIZE) + 1;
    int ytiles = ceill(dim.second / (float)TILE_SIZE) + 1;
    std::pair<int, int> ft = coorToTile(coor, zoom);
    std::pair<float, float> cf = coorToTileF(coor, zoom);
    int offX = TILE_SIZE * (cf.first - (int)cf.first);
    int offY = TILE_SIZE * (cf.second - (int)cf.second);
    for (int x = 0; x < xtiles; x++)
        for (int y = 0; y < ytiles; y++)
            img.draw_image(0, downloadTile({ft.first + x, ft.second + y}, zoom).data, x * TILE_SIZE - offX, y * TILE_SIZE - offY);
    return img;
}