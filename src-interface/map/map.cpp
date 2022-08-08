#include "map.h"
#include <math.h>
#define _USE_MATH_DEFINES
#include "common/utils.h"
#include <filesystem>
#include <chrono>

tileMap::tileMap(std::string url, std::string path, int expiry)
{
    tileServerURL = url;
    tileSaveDir = path;
    expiryTime = expiry;
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

tile tileMap::downloadTile(std::pair<int, int> t1, int zoom){
    image::Image<uint8_t> img;
    std::string filename = tileSaveDir + std::to_string(zoom) + "/" + std::to_string(t1.first) + "/" + std::to_string(t1.second) + ".png";
    bool old = false;
    if(std::filesystem::exists(filename)){
        std::filesystem::file_time_type ftime = std::filesystem::last_write_time(filename);
        int64_t t1 = std::chrono::duration_cast<std::chrono::hours>(ftime.time_since_epoch()).count();
        int64_t t2 = std::chrono::duration_cast<std::chrono::hours>(std::chrono::system_clock::now().time_since_epoch()).count();
        old = (t2-t1)/24 > expiryTime;
    }

    if (!std::filesystem::exists(filename) || old)
    {
        std::string res;
        std::string url = tileServerURL + std::to_string(zoom) + "/" + std::to_string(t1.first) + "/" + std::to_string(t1.second) + ".png";
        perform_http_request(url, res);
        img.load_png((uint8_t *)res.data(), res.size());
        std::filesystem::create_directories(tileSaveDir + std::to_string(zoom) + "/" + std::to_string(t1.first) + "/");
        img.save_png(filename);
    } else {
        img.load_png(filename);
    }
    return tile(t1.first, t1.second, img);
}

tile::tile(int x1, int y1, image::Image<uint8_t> tileImage){
    x = x1;
    y = y1;
    data = tileImage;
}