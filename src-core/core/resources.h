#pragma once

#include <string>

namespace resources
{
    bool resourceExists(std::string resource);
    std::string getResourcePath(std::string resource);
}