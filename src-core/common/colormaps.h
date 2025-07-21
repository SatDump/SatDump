#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace colormaps
{
    struct Map
    {
        std::string name;
        std::string author;
        float *map = nullptr;
        int entryCount = 0;
    };

    Map loadMap(std::string path);

    std::vector<uint32_t> generatePalette(Map map, int resolution);
}
