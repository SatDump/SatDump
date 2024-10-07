#include "gcp_compute.h"
#include "nlohmann/json_utils.h"
#include "../sat_proj/sat_proj.h"
// #include "logger.h"
#include "core/exception.h"

namespace satdump
{
    namespace gcp_compute
    {
        std::vector<satdump::projection::GCP> compute_gcps(nlohmann::ordered_json cfg, int width, int height)
        {
            if (cfg["type"] == "normal_gcps")
            {
                double ratio_x = 1, ratio_y = 1;
                if (width != -1)
                    ratio_x = round(cfg["width"].get<double>() / (double)width);
                if (height != -1)
                    ratio_y = round(cfg["height"].get<double>() / (double)height);

                std::vector<satdump::projection::GCP> gcps;
                int gcpn = cfg["gcp_cnt"];
                nlohmann::json gcps_cfg = cfg["gcps"];
                for (int i = 0; i < gcpn; i++)
                {
                    satdump::projection::GCP gcp;
                    gcp.x = gcps_cfg[i]["x"].get<double>() / ratio_x;
                    gcp.y = gcps_cfg[i]["y"].get<double>() / ratio_y;
                    gcp.lon = gcps_cfg[i]["lon"];
                    gcp.lat = gcps_cfg[i]["lat"];
                    gcps.push_back(gcp);
                }

                return gcps;
            }

            nlohmann::ordered_json mtd = cfg.contains("metadata") ? cfg["metadata"] : nlohmann::ordered_json();

            TLE tle;
            if (mtd.contains("tle"))
                tle = mtd["tle"];
            else
                throw satdump_exception("Could not get TLE!");

            nlohmann::ordered_json timestamps;
            if (mtd.contains("timestamps"))
                timestamps = mtd["timestamps"];
            else
                throw satdump_exception("Could not get timestamps!");

            std::vector<satdump::projection::GCP> gcps;

            std::shared_ptr<SatelliteProjection> projection = get_sat_proj(cfg, tle, timestamps);

            double ratio_x = 1, ratio_y = 1;
            if (width != -1)
                ratio_x = round((double)projection->img_size_x / (double)width);
            if (height != -1)
                ratio_y = round((double)projection->img_size_y / (double)height);

            int img_x_offset = 0;
            if (mtd.contains("img_x_offset"))
                img_x_offset = mtd["img_x_offset"];

            std::vector<int> values;
            if (cfg.contains("forced_gcps_x"))
                values = cfg["forced_gcps_x"].get<std::vector<int>>();
            for (int x = 0; x < projection->img_size_x; x += projection->gcp_spacing_x)
                values.push_back(x);
            values.push_back(projection->img_size_x - 1);

            if (cfg["type"] == "manual_single_line")
            {
                values.clear();

                for (auto item : cfg["points"].items())
                {
                    int px = std::stoi(item.key());
                    values.push_back(px);
                }
            }

            geodetic::geodetic_coords_t position;

            bool last_was_invalid = false;
            for (int y = 0; y < projection->img_size_y; y++)
            {
                for (int x : values)
                {
                    if (y % projection->gcp_spacing_y == 0 || y + 1 == (int)timestamps.size() || last_was_invalid)
                    {
                        if (projection->get_position(x + img_x_offset, y, position))
                        {
                            last_was_invalid = true;
                            continue;
                        }

                        if (position.lon != position.lon || position.lat != position.lat)
                            continue; // Check for NAN

                        // logger->trace("%f %f %f %f", (double)x / ratio_x, (double)y / ratio_y, (double)position.lon, (double)position.lat);
                        gcps.push_back({(double)x / ratio_x, (double)y / ratio_y, (double)position.lon, (double)position.lat});
                    }

                    last_was_invalid = false;
                }
            }

            return gcps;
        }
    }
}