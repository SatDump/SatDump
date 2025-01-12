#pragma once

#include "projection/raytrace/common/satellite_raytracer_sattrack.h"
#include "common/geodetic/euler_raytrace.h"
#include "nlohmann/json_utils.h"

#include "common/tracking/interpolator.h"

namespace jpss
{
    class VIIRSNormalLineSatProj : public satdump::proj::SatelliteRaytracerSatTrack
    {
    protected:
        std::vector<double> timestamps;

        double timestamp_offset;
        bool invert_scan;

        float roll_offset;
        float pitch_offset;
        float yaw_offset;

        // Pre-computed stuff
        std::vector<predict_position> sat_positions;

    public:
        VIIRSNormalLineSatProj(nlohmann::ordered_json cfg)
            : satdump::proj::SatelliteRaytracerSatTrack(cfg)
        {
            timestamps = cfg["timestamps"].get<std::vector<double>>();

            timestamp_offset = getValueOrDefault(cfg["timestamp_offset"], 0.0);
            invert_scan = getValueOrDefault(cfg["invert_scan"], false);

            roll_offset = getValueOrDefault(cfg["roll_offset"], 0.0);
            pitch_offset = getValueOrDefault(cfg["pitch_offset"], 0.0);
            yaw_offset = getValueOrDefault(cfg["yaw_offset"], 0.0);

            for (int i = 0; i < (int)timestamps.size(); i++)
            {
                double timestamp = timestamps[i] + timestamp_offset;
                sat_positions.push_back(sat_tracker->get_sat_position_at_raw(timestamp));
            }
        }

        bool get_position(double x, double y, geodetic::geodetic_coords_t &pos, double *otime)
        {
            if (y >= (int)timestamps.size())
                return 1;

            int iy = floor(y);

            double timestamp = timestamps[iy];

            if (timestamp == -1)
                return 1;

            auto pos_curr = sat_positions[iy];

            geodetic::euler_coords_t satellite_pointing;

            satellite_pointing.roll = (invert_scan ? -1.0 : 1.0) * x + roll_offset;
            satellite_pointing.pitch = pitch_offset;
            satellite_pointing.yaw = yaw_offset;

            geodetic::geodetic_coords_t ground_position;
            int ret = geodetic::raytrace_to_earth(pos_curr.time, pos_curr.position, pos_curr.velocity, satellite_pointing, ground_position);
            pos = ground_position.toDegs();

            if (ret)
                return 1;
            else
                return 0;
        }
    };
}