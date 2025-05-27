#include "core/resources.h"
#include <filesystem>
#include "satdump_vars.h"
#include "logger.h"

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
        {
            if (!std::filesystem::exists("resources/" + resource))
                logger->error("Resources " + resource + " does not exist!");
            return "resources/" + resource;
        }
        else
        {
            if (!std::filesystem::exists(satdump::RESPATH + "resources/" + resource))
                logger->error("Resources " + resource + " does not exist!");
            return satdump::RESPATH + "resources/" + resource;
        }
    }
}