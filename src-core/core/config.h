#pragma once

#include "nlohmann/json.hpp"
#include "dll_export.h"

namespace satdump
{
    namespace config
    {
        SATDUMP_DLL extern nlohmann::ordered_json master_cfg; // The main system-wise satdump_cfg.json
        SATDUMP_DLL extern nlohmann::ordered_json main_cfg;   // The actual config we should use, that includes user changes

        void loadConfig(std::string path, std::string user_path);
        void loadUserConfig(std::string user_path);
        void saveUserConfig();
    }
}