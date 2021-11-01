#pragma once
//#define BUILD_LIVE
//#define BUILD_ZIQ

#include <string>
#include <map>

namespace live
{
#ifdef BUILD_LIVE
    void initLiveProcessingMenu();
    void renderLiveProcessingMenu();

    extern std::string downlink_pipeline;
    extern std::string output_level;
    extern std::string output_file;
    extern std::map<std::string, std::string> parameters;
#endif
};
