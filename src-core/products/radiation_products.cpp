#include "radiation_products.h"
#include "logger.h"
#include "common/tracking/tracking.h"

namespace satdump
{
    void RadiationProducts::save(std::string directory)
    {
        type = "radiation";

        contents["counts"] = channel_counts;

        Products::save(directory);
    }

    void RadiationProducts::load(std::string file)
    {
        Products::load(file);
        std::string directory = std::filesystem::path(file).parent_path();

        channel_counts = contents["counts"].get<std::vector<std::vector<int>>>();
    }

    void make_radiation_map(RadiationProducts &products, int channel, image::Image<uint16_t> &initial_map, int radius, image::Image<uint16_t> color_lut, int min, int max)
    {
        int img_x = initial_map.width();
        int img_y = initial_map.height();

        int lut_size = color_lut.width();
        std::vector<double> timestamps = products.get_timestamps(channel);

        SatelliteTracker tracker(products.tle);

        for (int samplec = 0; samplec < products.channel_counts[channel].size(); samplec++)
        {
            int value = (double(products.channel_counts[channel][samplec] - min) / max) * lut_size;

            // logger->info("{:d} {:d}", products.channel_counts[channel][samplec], value);

            if (value < 0)
                value = 0;
            if (value > lut_size - 1)
                value = lut_size - 1;

            geodetic::geodetic_coords_t satpos = tracker.get_sat_position_at(timestamps[samplec]);

            // Scale to the map
            int image_y = img_y - ((90.0f + satpos.lat) / 180.0f) * img_y;
            int image_x = (satpos.lon / 360) * img_x + (img_x / 2);

            uint16_t color[] = {color_lut.channel(0)[value], color_lut.channel(1)[value], color_lut.channel(2)[value]};

            initial_map.draw_circle(image_x, image_y, radius, color, true);
        }
    }
}