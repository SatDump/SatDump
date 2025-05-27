#include "satellite_raytracer_sattrack.h"
#include "core/exception.h"

namespace satdump
{
    namespace projection
    {
        SatelliteRaytracerSatTrack::SatelliteRaytracerSatTrack(nlohmann::json cfg)
            : SatelliteRaytracer(cfg)
        {
            if (cfg.contains("ephemeris") && cfg["ephemeris"].size() > 1)
                sat_tracker = std::make_shared<satdump::SatelliteTracker>((nlohmann::json)cfg["ephemeris"]);
            else if (cfg.contains("tle") && cfg["tle"].get<TLE>().norad != -1)
                sat_tracker = std::make_shared<satdump::SatelliteTracker>(cfg["tle"].get<TLE>());
            else
                throw satdump_exception("No TLE or ephemeris provided!");
        }
    }
}