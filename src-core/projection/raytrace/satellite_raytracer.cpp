#include "satellite_raytracer.h"
#include "core/exception.h"
#include "core/plugin.h"

// TODOREWORK
#include "projection/raytrace/timestamp_filtering.h"

//
#include "common/manual_line.h"
#include "common/normal_line.h"
#include "common/timestamp_line_gcps.h"
#include "projection/raytrace/common/normal_per_ifov.h"

namespace satdump
{
    namespace projection
    {
        // TODOREWORK Are we keeping this?
        void try_interpolate_timestamps(std::vector<double> &timestamps, nlohmann::json &cfg)
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

        std::shared_ptr<SatelliteRaytracer> get_satellite_raytracer(nlohmann::json cfg)
        {
            if (cfg.contains("timestamps"))
            {
                auto tm = cfg["timestamps"].get<std::vector<double>>();
                if (cfg.contains("timefilter"))
                    tm = timestamp_filtering::filter_timestamps_width_cfg(tm, cfg["timefilter"]); // TODOREWORK move to pre-processing?
                try_interpolate_timestamps(tm, cfg);
                cfg["timestamps"] = tm;
            }

            std::string type = cfg["type"];

            if (type == "normal_single_line")
                return std::make_shared<NormalLineRaytracer>(cfg);
            else if (type == "normal_single_line_old")
                return std::make_shared<NormalLineRaytracerOld>(cfg);
            else if (type == "manual_single_line_old")
                return std::make_shared<ManualLineRaytracerOld>(cfg);
            else if (type == "normal_per_ifov_old")
                return std::make_shared<NormalPerIFOVRaytracerOld>(cfg);
            else if (type == "gcps_timestamps_line")
                return std::make_shared<TimestampLineGCPsRaytracer>(cfg);

            // And plugins!
            std::vector<std::shared_ptr<SatelliteRaytracer>> projs;
            satdump::eventBus->fire_event<RequestSatelliteRaytracerEvent>({type, projs, cfg});
            if (projs.size() > 0)
                return projs[0];

            throw satdump_exception("Invalid satellite projection : " + type + "!");
        }
    } // namespace proj
} // namespace satdump