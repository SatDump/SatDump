#include "resources.h"
#include <filesystem>
#include "satdump_vars.h"

namespace resources
{
    bool resourceExists(std::string resource)
    {
        if (std::filesystem::exists("resources"))
            return std::filesystem::exists("resources/" + resource);
        else
            return std::filesystem::exists(satdump::RESPATH + "resources/" + resource);
    }

    std::string getResourcePath(std::string resource)
    {
        if (std::filesystem::exists("resources"))
            return "resources/" + resource;
        else
            return satdump::RESPATH + "resources/" + resource;
    }
}