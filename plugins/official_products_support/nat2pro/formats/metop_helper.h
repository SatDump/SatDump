#pragma once

#include "common/tracking/tle.h"

namespace nat2pro
{
    struct MetOpSatInfo
    {
        std::string sat_name;
        int norad = 0;
        std::optional<satdump::TLE> satellite_tle;
    };

    inline MetOpSatInfo getMetOpSatInfoFromID(std::string sat_id, double prod_timestamp)
    {
        MetOpSatInfo info;

        info.sat_name = "Unknown MetOp";
        if (sat_id == "M02")
            info.sat_name = "MetOp-A";
        else if (sat_id == "M01")
            info.sat_name = "MetOp-B";
        else if (sat_id == "M03")
            info.sat_name = "MetOp-C";

        info.norad = 0;
        if (sat_id == "M02")
            info.norad = 29499;
        else if (sat_id == "M01")
            info.norad = 38771;
        else if (sat_id == "M03")
            info.norad = 43689;

        info.satellite_tle = satdump::general_tle_registry->get_from_norad_time(info.norad, prod_timestamp);

        return info;
    }
}