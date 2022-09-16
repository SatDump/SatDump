#pragma once

#include <string>
#include "nlohmann/json.hpp"
#include <optional>
#include "dll_export.h"

namespace satdump
{
    // Simple struct to hold TLE definitions
    struct TLE
    {
        int norad;
        std::string name;
        std::string line1;
        std::string line2;
    };

    inline void to_json(nlohmann::json &j, const TLE &v)
    {
        j["norad"] = v.norad;
        j["name"] = v.name;
        j["line1"] = v.line1;
        j["line2"] = v.line2;
    }

    inline void from_json(const nlohmann::json &j, TLE &v)
    {
        v.norad = j["norad"].get<int>();
        v.name = j["name"].get<std::string>();
        v.line1 = j["line1"].get<std::string>();
        v.line2 = j["line2"].get<std::string>();
    }

    // TLE Registry class, which simply extends a std::vector with some QOL functions
    class TLERegistry : public std::vector<TLE>
    {
    public:
        std::optional<TLE> get_from_norad(int norad)
        {
            std::vector<TLE>::iterator it = std::find_if(begin(),
                                                         end(),
                                                         [&norad](const TLE &e)
                                                         {
                                                             return e.norad == norad;
                                                         });

            if (it != end())
                return std::optional<TLE>(*it);
            else
                return std::optional<TLE>();
        };
    };

    SATDUMP_DLL extern TLERegistry general_tle_registry;

    void updateTLEFile(std::string path);
    void loadTLEFileIntoRegistry(std::string path);
}
