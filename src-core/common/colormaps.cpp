#include <filesystem>
#include <fstream>
#include "colormaps.h"
#include "nlohmann/json.hpp"

namespace colormaps
{
    Map loadMap(std::string path)
    {
        std::ifstream file(path.c_str());
        nlohmann::json data;
        file >> data;
        file.close();

        Map map;
        std::vector<std::string> mapTxt;

        try
        {
            map.name = data["name"];
            map.author = data["author"];
            mapTxt = data["map"].get<std::vector<std::string>>();
        }
        catch (const std::exception &)
        {
            // spdlog::error("Could not load {0}", path);
            return map;
        }

        map.entryCount = mapTxt.size();
        map.map = new float[mapTxt.size() * 3];
        int i = 0;
        for (auto const &col : mapTxt)
        {
            // uint8_t r, g, b, a;
            map.map[i * 3] = std::stoi(col.substr(1, 2), NULL, 16);
            map.map[(i * 3) + 1] = std::stoi(col.substr(3, 2), NULL, 16);
            map.map[(i * 3) + 2] = std::stoi(col.substr(5, 2), NULL, 16);
            i++;
        }

        return map;
    }

    std::vector<uint32_t> generatePalette(Map map, int resolution)
    {
        int colorCount = map.entryCount;

        std::vector<uint32_t> waterfallPallet(resolution);

        for (int i = 0; i < resolution; i++)
        {
            int lowerId = floorf(((float)i / (float)resolution) * colorCount);
            int upperId = ceilf(((float)i / (float)resolution) * colorCount);
            lowerId = std::clamp<int>(lowerId, 0, colorCount - 1);
            upperId = std::clamp<int>(upperId, 0, colorCount - 1);
            float ratio = (((float)i / (float)resolution) * colorCount) - lowerId;
            float r = (map.map[(lowerId * 3) + 0] * (1.0 - ratio)) + (map.map[(upperId * 3) + 0] * (ratio));
            float g = (map.map[(lowerId * 3) + 1] * (1.0 - ratio)) + (map.map[(upperId * 3) + 1] * (ratio));
            float b = (map.map[(lowerId * 3) + 2] * (1.0 - ratio)) + (map.map[(upperId * 3) + 2] * (ratio));
            waterfallPallet[i] = ((uint32_t)255 << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r;
        }

        return waterfallPallet;
    }
}