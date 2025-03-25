#include "gcp_compute.h"
#include "core/exception.h"

// #include "logger.h"

namespace satdump
{
    namespace proj
    {
        std::vector<satdump::projection::GCP> compute_gcps(Projection p)
        {
            // Get Raw CFG
            nlohmann::json cfg = p;

            // We need image width/height
            int width = -1, height = -1;

            if (cfg.contains("width"))
                width = cfg["width"];
            else
                throw satdump_exception("Image width must be present!");

            if (cfg.contains("height"))
                height = cfg["height"];
            else
                throw satdump_exception("Image height must be present!");

            std::vector<satdump::projection::GCP> gcps;

            // Get GCP configs
            int gcp_spacing_x = -1, gcp_spacing_y = -1;

            if (cfg.contains("gcp_spacing_x"))
                gcp_spacing_x = cfg["gcp_spacing_x"];
            else
                throw satdump_exception("GCP Spacing X must be present!");

            if (cfg.contains("gcp_spacing_y"))
                gcp_spacing_y = cfg["gcp_spacing_y"];
            else
                throw satdump_exception("GCP Spacing Y must be present!");

            // Generate x table of points we want to generate
            std::vector<int> values;
            if (cfg.contains("forced_gcps_x")) // They may be some that are imposed, due to instrument specifics (eg, VIIRS)
                values = cfg["forced_gcps_x"].get<std::vector<int>>();
            for (int x = 0; x < width; x += gcp_spacing_x)
                values.push_back(x);
            values.push_back(width - 1);

            // TODOREWORK Handle this properly. Boolean or function in the raytracer?
            if (cfg["type"] == "manual_single_line")
            {
                values.clear();

                for (auto item : cfg["points"].items())
                {
                    int px = std::stoi(item.key());
                    values.push_back(px);
                }
            }

            if (!p.init(0, 1))
                throw satdump_exception("Could not setup projection to calculate GCPs!");

            // Calculate actual GCPs
            geodetic::geodetic_coords_t position;
            bool last_was_invalid = false;

            for (int y = 0; y < height; y++)
            {
                for (int x : values)
                {
                    if (y % gcp_spacing_y == 0 || y + 1 == height || last_was_invalid)
                    {
                        if (p.inverse(x, y, position))
                        { // Check for invalid position (pointing to space?)
                            last_was_invalid = true;
                            continue;
                        }

                        if (position.lon != position.lon || position.lat != position.lat)
                        { // Check for NAN
                            last_was_invalid = true;
                            continue;
                        }

                        // logger->trace("%f %f %f %f", (double)x, (double)y, (double)position.lon, (double)position.lat);
                        gcps.push_back({(double)x, (double)y, (double)position.lon, (double)position.lat});
                    }

                    last_was_invalid = false;
                }
            }

            return gcps;
        }
    }
}