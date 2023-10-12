#include "sat_proj.h"
#include "common/geodetic/euler_raytrace.h"
#include "common/geodetic/vincentys_calculations.h"
#include "nlohmann/json_utils.h"

#include "common/projection/thinplatespline.h"
#include "logger.h"

#include "common/projection/timestamp_filtering.h"

namespace satdump
{
    void try_interpolate_timestamps(std::vector<double> &timestamps, nlohmann::ordered_json &cfg)
    {
        if (cfg.contains("interpolate_timestamps"))
        {
            int to_interp = cfg["interpolate_timestamps"];
            float scantime = cfg["interpolate_timestamps_scantime"];

            auto timestamp_copy = timestamps;
            timestamps.clear();
            for (auto timestamp : timestamp_copy)
            {
                for (int i = -(to_interp / 2); i < (to_interp / 2); i++)
                {
                    if (timestamp != -1)
                        timestamps.push_back(timestamp + i * scantime);
                    else
                        timestamps.push_back(-1);
                }
            }
        }
    }

#include "normal_line_proj.h"

#include "normal_per_ifov_proj.h"

#include "manual_line_proj.h"

    std::shared_ptr<SatelliteProjection> get_sat_proj(nlohmann::ordered_json cfg, TLE tle, std::vector<double> timestamps_raw)
    {
        if (cfg.contains("timefilter"))
            timestamps_raw = timestamp_filtering::filter_timestamps_width_cfg(timestamps_raw, cfg["timefilter"]);

        if (cfg["type"].get<std::string>() == "normal_single_line")
            return std::make_shared<NormalLineSatProj>(cfg, tle, timestamps_raw);
        else if (cfg["type"].get<std::string>() == "normal_single_line_old")
            return std::make_shared<NormalLineSatProjOld>(cfg, tle, timestamps_raw);

        else if (cfg["type"].get<std::string>() == "normal_per_ifov")
            return std::make_shared<NormalPerIFOVProj>(cfg, tle, timestamps_raw);
        else if (cfg["type"].get<std::string>() == "normal_per_ifov_old")
            return std::make_shared<NormalPerIFOVProjOld>(cfg, tle, timestamps_raw);

        else if (cfg["type"].get<std::string>() == "manual_single_line")
            return std::make_shared<NormalLineManualSatProj>(cfg, tle, timestamps_raw);

        throw std::runtime_error("Invalid satellite projection!");
    }
}