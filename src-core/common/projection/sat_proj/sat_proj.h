#pragma once

#include <memory>
#include "nlohmann/json.hpp"
#include "common/geodetic/geodetic_coordinates.h"
#include "common/tracking/tracking.h"

namespace satdump
{
    void try_interpolate_timestamps(std::vector<double> &timestamps, nlohmann::ordered_json &cfg);

    class SatelliteProjection
    {
    protected:
        const nlohmann::ordered_json cfg;
        const TLE tle;
        const nlohmann::ordered_json timestamps_raw;
        std::shared_ptr<satdump::SatelliteTracker> sat_tracker;

    public:
        int img_size_x;
        int img_size_y;

        int gcp_spacing_x;
        int gcp_spacing_y;

    public:
        SatelliteProjection(nlohmann::ordered_json cfg, TLE tle, nlohmann::ordered_json timestamps_raw)
            : cfg(cfg),
              tle(tle),
              timestamps_raw(timestamps_raw)
        {
            if (cfg.contains("ephemeris") && cfg["ephemeris"].size() > 1)
                sat_tracker = std::make_shared<satdump::SatelliteTracker>((nlohmann::json)cfg["ephemeris"]);
            else
                sat_tracker = std::make_shared<satdump::SatelliteTracker>(tle);
        }

        virtual bool get_position(int x, int y, geodetic::geodetic_coords_t &pos) = 0;
    };

    struct RequestSatProjEvent
    {
        std::string id;
        std::vector<std::shared_ptr<SatelliteProjection>> &projs;
        nlohmann::ordered_json cfg;
        TLE tle;
        nlohmann::ordered_json timestamps_raw;
    };

    std::shared_ptr<SatelliteProjection> get_sat_proj(nlohmann::ordered_json cfg, TLE tle, std::vector<double> timestamps_raw);
}