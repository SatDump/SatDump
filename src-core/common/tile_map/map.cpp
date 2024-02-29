#include "map.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include "common/utils.h"
#include <filesystem>
#include <chrono>
#include "logger.h"

tileMap::tileMap(std::string url, std::string path, int expiry)
{
    tileServerURL = url;
    tileSaveDir = path;
    expiryTime = expiry;
}

std::pair<int, int> tileMap::coorToTile(std::pair<float, float> coor, int zoom)
{
    logger->debug("Calculating tile coordinates!");
    int x, y;
    int n = std::pow(2, zoom);
    x = n * ((coor.second + 180) / 360);
    y = n * (1 - (log(tan(coor.first * (M_PI / 180)) + 1 / cos(coor.first * (M_PI / 180))) / M_PI)) / 2;
    return {x, y};
}

std::pair<float, float> tileMap::coorToTileF(std::pair<float, float> coor, int zoom)
{
    logger->debug("Calculating tile coordinates (float)!");
    float x, y;
    float n = std::pow(2, zoom);
    x = n * ((coor.second + 180.0) / 360.0);
    y = n * (1.0 - (log(tan(coor.first * (M_PI / 180.0)) + 1 / cos(coor.first * (M_PI / 180.0))) / M_PI)) / 2.0;
    return {x, y};
}

int widthToZoom(float deg, int width)
{
    logger->debug("Calculating possible width!");
    return round(log2(360 * width / (deg * TILE_SIZE)));
}

mapTile tileMap::downloadTile(std::pair<int, int> t1, int zoom)
{
    logger->debug("Downloading tile (" + std::to_string(t1.first) + ", " + std::to_string(t1.second) + ")");
    image::Image<uint8_t> img;
    if (t1.first >= pow(2, zoom))
        t1.first -= ((t1.first / (int)pow(2, zoom)) * pow(2, zoom));
    if (t1.second >= pow(2, zoom))
    {
        logger->debug("Invalid tile, returning blank");
        img = image::Image<uint8_t>(TILE_SIZE, TILE_SIZE, 3);
        img.fill(120);
        return {t1.first, t1.second, img};
    }
    std::string filename = tileSaveDir + std::to_string(zoom) + "/" + std::to_string(t1.first) + "/" + std::to_string(t1.second);
    bool old = false;

    if (std::filesystem::exists(filename))
    {
        std::filesystem::file_time_type file = std::filesystem::last_write_time(filename);
        std::filesystem::file_time_type system = std::filesystem::file_time_type::clock::now();
        std::chrono::duration<int64_t, std::nano> dur = system.time_since_epoch() - file.time_since_epoch();
        old = dur.count() / 86400000000000 > expiryTime;
    }

    if (!std::filesystem::exists(filename) || old)
    {
        std::string url = tileServerURL;
        std::string res;

        url.replace(url.find("{z}"), 3, std::to_string(zoom));
        url.replace(url.find("{x}"), 3, std::to_string(t1.first));
        url.replace(url.find("{y}"), 3, std::to_string(t1.second));

        logger->debug("Tile not found or is outdated, downloading from: " + url);

        perform_http_request(url, res);
        img.load_img((uint8_t *)res.data(), res.size());
        // system(std::string("wget " + url + " -O /tmp/img.jpg").c_str());
        // img.load_img("/tmp/img.jpg");

        std::filesystem::create_directories(tileSaveDir + std::to_string(zoom) + "/" + std::to_string(t1.first) + "/");
        std::ofstream of(filename, std::ios::binary);
        of << res;
        of.close();
        logger->debug("Saving tile to: " + filename);
    }
    else
    {
        logger->debug("Tile found, loading from disk");
        img.load_img(filename);
    }
    return mapTile(t1.first, t1.second, img);
}

mapTile::mapTile(int x1, int y1, image::Image<uint8_t> tileImage)
{
    x = x1;
    y = y1;
    data = tileImage;
}

image::Image<uint8_t> tileMap::getMapImage(std::pair<float, float> coor, int zoom, std::pair<int, int> dim, float *progress)
{
    logger->debug("Creating map image");
    image::Image<uint8_t> img(dim.first, dim.second, 3);
    int xtiles = ceill(dim.first / (float)TILE_SIZE) + 1;
    int ytiles = ceill(dim.second / (float)TILE_SIZE) + 1;
    std::pair<int, int> ft = coorToTile(coor, zoom);
    std::pair<float, float> cf = coorToTileF(coor, zoom);
    int offX = TILE_SIZE * (cf.first - (int)cf.first);
    int offY = TILE_SIZE * (cf.second - (int)cf.second);
    for (int x = 0; x < xtiles; x++)
        for (int y = 0; y < ytiles; y++)
        {
            img.draw_image(0, downloadTile({ft.first + x, ft.second + y}, zoom).data, x * TILE_SIZE - offX, y * TILE_SIZE - offY);
            if (progress != nullptr)
                *progress = float(x * xtiles + y) / (xtiles * ytiles);
        }
    return img;
}

image::Image<uint8_t> tileMap::getMapImage(std::pair<float, float> coor, std::pair<float, float> coor1, int zoom, float *progress)
{
    logger->debug("Creating map image");
    std::pair<float, float> cf, cf1;
    int xtiles, ytiles;
    do
    {
        cf = coorToTileF(coor, zoom);
        cf1 = coorToTileF(coor1, zoom);
        xtiles = ceil(abs(cf.first - cf1.first)) + 1;
        ytiles = ceil(abs(cf.second - cf1.second)) + 1;
        if (xtiles * ytiles > TILE_DL_LIMIT && zoom >= 13)
        {
            logger->warn("Requested area is over 250 tiles with zoom > 13, lowering zoom level.");
            zoom--;
        }
    } while (xtiles * ytiles > TILE_DL_LIMIT && zoom >= 13);

    int offX = TILE_SIZE * (std::min(cf.first, cf1.first) - (float)(int)std::min(cf.first, cf1.first));
    int offY = TILE_SIZE - (TILE_SIZE * (std::min(cf.second, cf1.second) - (float)(int)std::min(cf.second, cf1.second)));
    image::Image<uint8_t> img(std::round(abs(cf.first - cf1.first) * TILE_SIZE), std::round(abs(cf.second - cf1.second) * TILE_SIZE), 3);
    for (int x = 0; x < xtiles; x++)
        for (int y = 0; y < ytiles; y++)
        {
            img.draw_image(0, downloadTile({std::min(cf.first, cf1.first) + (float)x, std::min(cf.second, cf1.second) + (float)y}, zoom).data, x * TILE_SIZE - offX, y * TILE_SIZE - offY);
            if (progress != nullptr)
                *progress = float(x * xtiles + y) / (xtiles * ytiles);
        }
    return img;
}