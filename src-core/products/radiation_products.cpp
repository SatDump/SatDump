#include "radiation_products.h"
#include "logger.h"
#include "common/tracking/tracking.h"
#include "resources.h"
#include <filesystem>
#include "common/image/io.h"
#include "common/image/image_lut.h"

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
        std::string directory = std::filesystem::path(file).parent_path().string();

        channel_counts = contents["counts"].get<std::vector<std::vector<int>>>();
    }

    image::Image make_radiation_map(RadiationProducts &products, RadiationMapCfg cfg, bool isOverlay, float *progress)
    {
        image::Image map;
        if (!isOverlay)
            image::load_jpeg(map, resources::getResourcePath("maps/nasa.jpg").c_str());
        else
            map.init(8, 2048, 1024, 4);
        image::Image color_lut = image::LUT_jet<uint16_t>();

        int img_x = map.width();
        int img_y = map.height();
        int channel = cfg.channel - 1;

        if (!products.has_timestamps(channel))
            return map;

        int lut_size = color_lut.width();
        std::vector<double> timestamps = products.get_timestamps(channel);

        // logger->critical(timestamps.size());

        SatelliteTracker tracker(products.tle);

        for (int samplec = 0; samplec < (int)products.channel_counts[channel].size(); samplec++)
        {
            int value = (double(products.channel_counts[channel][samplec] - cfg.min) / cfg.max) * lut_size;

            // logger->info("%d %d", products.channel_counts[channel][samplec], value);

            if (value < 0)
                value = 0;
            if (value > lut_size - 1)
                value = lut_size - 1;

            geodetic::geodetic_coords_t satpos = tracker.get_sat_position_at(timestamps[samplec]);

            // Scale to the map
            int image_y = img_y - ((90.0f + satpos.lat) / 180.0f) * img_y;
            int image_x = (satpos.lon / 360) * img_x + (img_x / 2);

            std::vector<double> color = {color_lut.getf(0, value), color_lut.getf(1, value), color_lut.getf(2, value), 1};

            map.draw_circle(image_x % img_x, image_y, cfg.radius, color, true);

            if (progress != nullptr)
                *progress = float(samplec) / float(products.channel_counts[channel].size());
        }

        return map;
    }
}