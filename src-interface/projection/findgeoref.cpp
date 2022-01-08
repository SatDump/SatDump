#include "findgeoref.h"
#include <filesystem>
#include <vector>

std::optional<std::string> findGeoRefForImage(std::string image_path)
{
    std::string directory = std::filesystem::path(image_path).parent_path().string();
    std::string filename = std::filesystem::path(image_path).filename().string();

    std::vector<std::filesystem::path> georefs;

    for (auto &p : std::filesystem::recursive_directory_iterator(directory))
    {
        if (p.path().extension() == ".georef")
            georefs.push_back(p.path());
    }

    // If there is none, exit
    if (georefs.size() <= 0)
        return std::optional<std::string>();
    // If there is only one, return that one
    else if (georefs.size() == 1)
        return std::optional<std::string>(georefs[0].string());
    // If there is more than one, try to match it by filename
    else if (georefs.size() > 1)
    {
        for (std::filesystem::path &georef : georefs)
        {
            std::string name = georef.filename().string().substr(0, georef.filename().string().find_last_of("."));
            if (filename.find(name) != std::string::npos)
                return std::optional<std::string>(georef.string());
        }

        return std::optional<std::string>();
    }
    // Otherwise, exit
    else
        return std::optional<std::string>();
}