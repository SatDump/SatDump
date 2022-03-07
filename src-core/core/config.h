#pragma once

#include "nlohmann/json.hpp"

namespace satdump
{
    namespace config
    {
        extern nlohmann::ordered_json cfg;

        void loadConfig(std::string path);
    }
}