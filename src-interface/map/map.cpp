#include "map.h"
#include <math.h>
#define _USE_MATH_DEFINES
#include "common/utils.h"
#include <iostream>
#include <fstream>
#include <filesystem>

tileMap::tileMap(std::string url, std::string path)
{
    tileServerURL = url;
    tileSaveDir = path;
}

std::pair<int, int> tileMap::coorToTile(float lat, float lon, int zoom)
{
    int x, y;
    int n = std::pow(2, zoom);
    x = n * ((lon + 180) / 360);
    y = n * (1 - (log(tan(lat * (M_PI / 180)) + 1 / cos(lat * (M_PI / 180))) / M_PI)) / 2;
    downloadTile({x, y}, zoom);
    return {x, y};
}

void tileMap::downloadTile(std::pair<int, int> t1, int zoom){
    if (!std::filesystem::exists(tileSaveDir + std::to_string(zoom) + "/" + std::to_string(t1.first) + "/" + std::to_string(t1.second) + ".png"))
    {
        std::string res;
        std::string url = "http://tile.openstreetmap.org/" + std::to_string(zoom) + "/" + std::to_string(t1.first) + "/" + std::to_string(t1.second) + ".png";
        std::cout << url << std::endl;
        perform_http_request(url, res);
        //image::Image<uint8_t> img((uint8_t *)&res, 256, 256, 3);
        //img.save_png("tile.png");
        std::filesystem::create_directories(tileSaveDir + std::to_string(zoom) + "/" + std::to_string(t1.first) + "/");
        std::ofstream out(tileSaveDir + std::to_string(zoom) + "/" + std::to_string(t1.first) + "/" + std::to_string(t1.second) + ".png");
        out << res;
        out.close();
    }
}