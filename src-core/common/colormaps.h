#pragma once
#include <string>
#include <vector>
#include <map>

namespace colormaps
{
    struct Map
    {
        std::string name;
        std::string author;
        float *map;
        int entryCount;
    };

    Map loadMap(std::string path);

    std::vector<uint32_t> generatePalette(Map map, int resolution);
}