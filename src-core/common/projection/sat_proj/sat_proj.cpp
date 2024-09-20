#include "sat_proj.h"
#include "common/geodetic/euler_raytrace.h"
#include "common/geodetic/vincentys_calculations.h"
#include "nlohmann/json_utils.h"
#include "core/exception.h"

#include "common/projection/thinplatespline.h"
#include "logger.h"

#include "common/projection/timestamp_filtering.h"

#include "core/plugin.h"

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

    StandardSatProj::StandardSatProj(nlohmann::ordered_json cfg, TLE tle, nlohmann::ordered_json timestamps_raw)
        : SatelliteProjection(cfg, tle, timestamps_raw)
    {
        bool proj_err = false;
        try
        {
            p = cfg;
        }
        catch (std::exception &)
        {
            proj_err = true;
        }

        if (proj::projection_setup(&p) || proj_err)
        {
            logger->critical(cfg.dump(4));
            throw satdump_exception("Invalid standard satellite projection!");
        }
    }

    bool StandardSatProj::get_position(int x, int y, geodetic::geodetic_coords_t &pos)
    {
        return proj::projection_perform_inv(&p, x, y, &pos.lon, &pos.lat);
    }

#include "normal_line_proj.h"

#include "normal_per_ifov_proj.h"

#include "manual_line_proj.h"

#include "normal_line_xy_proj.h"

    std::shared_ptr<SatelliteProjection> get_sat_proj(nlohmann::ordered_json cfg, TLE tle, std::vector<double> timestamps_raw, bool allow_standard)
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

        else if (cfg["type"].get<std::string>() == "normal_single_xy_line")
            return std::make_shared<NormalLineXYSatProj>(cfg, tle, timestamps_raw);

        // And plugins!
        std::vector<std::shared_ptr<SatelliteProjection>> projs;
        satdump::eventBus->fire_event<RequestSatProjEvent>({cfg["type"].get<std::string>(), projs, cfg, tle, timestamps_raw});
        if (projs.size() > 0)
            return projs[0];

        if (cfg["type"].get<std::string>() == "geos" ||
            allow_standard)
            return std::make_shared<StandardSatProj>(cfg, tle, timestamps_raw);

        throw satdump_exception("Invalid satellite projection!");
    }
}