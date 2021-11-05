#pragma once
//#define BUILD_LIVE
//#define BUILD_ZIQ

#include <string>
#include <map>
#include "nlohmann/json.hpp"

namespace live
{
#ifdef BUILD_LIVE
    void initLiveProcessingMenu();
    void renderLiveProcessingMenu();

    extern std::string downlink_pipeline;
    extern std::string output_level;
    extern std::string output_file;
    extern nlohmann::json parameters;
#endif
};
