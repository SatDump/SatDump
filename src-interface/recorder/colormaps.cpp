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
            //uint8_t r, g, b, a;
            map.map[i * 3] = std::stoi(col.substr(1, 2), NULL, 16);
            map.map[(i * 3) + 1] = std::stoi(col.substr(3, 2), NULL, 16);
            map.map[(i * 3) + 2] = std::stoi(col.substr(5, 2), NULL, 16);
            i++;
        }

        return map;
    }
}