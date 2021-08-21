#include "resources.h"
#include <filesystem>
#include "satdump_vars.h"

namespace resources
{
    std::string res_root = (std::string)RESOURCES_PATH;

    bool resourceExists(std::string resource)
    {
        if (std::filesystem::exists("resources"))
            return std::filesystem::exists("resources/" + resource);
        else
            return std::filesystem::exists(res_root + "resources/" + resource);
    }

    std::string getResourcePath(std::string resource)
    {
        if (std::filesystem::exists("resources"))
            return "resources/" + resource;
        else
            return res_root + "resources/" + resource;
    }
}