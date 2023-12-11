#pragma once

#include <vector>
#include "common/tracking/tle.h"

namespace satdump
{
    struct SatellitePass
    {
        int norad;
        double aos_time;
        double los_time;
        float max_elevation;
    };

    std::vector<SatellitePass> getPassesForSatellite(int norad, double initial_time, double timespan, double qth_lon, double qth_lat, double qth_alt, std::vector<SatellitePass> premade_passes = {});

    std::vector<SatellitePass> filterPassesByElevation(std::vector<SatellitePass> passes, float min_elevation, float max_elevation);

    std::vector<SatellitePass> selectPassesForAutotrack(std::vector<SatellitePass> passes);
}