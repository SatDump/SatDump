#include "json.hpp"
#include <fstream>
#include <filesystem>

nlohmann::json loadJsonFile(std::string path)
{
    nlohmann::json j;

    if (std::filesystem::exists(path))
    {
        std::ifstream istream(path);
        istream >> j;
        istream.close();
    }

    return j;
}

void saveJsonFile(std::string path, nlohmann::json j)
{
    std::ofstream ostream(path);
    ostream << j.dump(4);
    ostream.close();
}