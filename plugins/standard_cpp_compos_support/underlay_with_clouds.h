#pragma once

#include "products/image_products.h"
#include "resources.h"
#define DEFINE_COMPOSITE_UTILS 1
#include "common/image/composite.h"

#include "common/projection/sat_proj/sat_proj.h"
#include "common/projection/projs/equirectangular.h"

namespace cpp_compos
{
    image::Image<uint16_t> underlay_with_clouds(satdump::ImageProducts *img_pro,
                                                std::vector<image::Image<uint16_t>> &inputChannels,
                                                std::vector<std::string> channelNumbers,
                                                std::string cpp_id,
                                                nlohmann::json vars,
                                                nlohmann::json offsets_cfg,
                                                std::vector<double> *final_timestamps = nullptr,
                                                float *progress = nullptr)
    {
        image::compo_cfg_t f = image::get_compo_cfg(inputChannels, channelNumbers, offsets_cfg);

        image::Image<uint8_t> img_background;
        img_background.load_img(resources::getResourcePath("maps/nasa_hd.jpg"));
        geodetic::projection::EquirectangularProjection equp;
        equp.init(img_background.width(), img_background.height(), -180, 90, 180, -90);
        int map_x, map_y;

        // return 3 channels, RGB
        image::Image<uint16_t> rgb_output(f.maxWidth, f.maxHeight, 3);
        uint16_t *channelVals = new uint16_t[inputChannels.size()];

        geodetic::geodetic_coords_t coords;
        auto projFunc = satdump::get_sat_proj(img_pro->get_proj_cfg(), img_pro->get_tle(), *final_timestamps, true);

        float cfg_offset = vars["minoffset"];
        float cfg_scalar = vars["scalar"];
        float cfg_thresold = vars["thresold"];
        bool cfg_blend = vars["blend"];
        bool cfg_invert = vars["invert"];

        size_t width = rgb_output.width();
        size_t height = rgb_output.height();
        size_t background_size = img_background.width() * img_background.height();

        auto &ch_equal = inputChannels[0];
        ch_equal.equalize();
        float val = 0;

        for (size_t x = 0; x < rgb_output.width(); x++)
        {
            for (size_t y = 0; y < rgb_output.height(); y++)
            {
                if (!projFunc->get_position(x, y, coords))
                {
                    equp.forward(coords.lon, coords.lat, map_x, map_y);

                    if (cfg_invert)
                        val = 1.0 - ((ch_equal[(y * width) + x] / 65535.0) - cfg_offset) * cfg_scalar;
                    else
                        val = ((ch_equal[(y * width) + x] / 65535.0) - cfg_offset) * cfg_scalar;

                    if (val > 1)
                        val = 1;
                    if (val < 0)
                        val = 0;

                    int mappos = map_y * img_background.width() + map_x;

                    if (mappos >= background_size)
                        mappos = background_size - 1;

                    if (mappos < 0)
                        mappos = 0;

                    if (cfg_blend == 1)
                    {
                        for (int c = 0; c < 3; c++)
                        {
                            float mval = img_background[background_size * c + mappos] / 255.0;
                            float fval = mval * (1.0 - val) + val * val;
                            rgb_output.channel(c)[y * rgb_output.width() + x] = rgb_output.clamp(fval * 65535);
                        }
                    }
                    else
                    {
                        if (cfg_thresold == 0)
                        {
                            for (int c = 0; c < 3; c++)
                            {
                                float mval = img_background[background_size * c + mappos] / 255.0;
                                float fval = mval * 0.4 + val * 0.6;
                                rgb_output.channel(c)[y * rgb_output.width() + x] = rgb_output.clamp(fval * 65535);
                            }
                        }
                        else
                        {
                            if (val < cfg_thresold)
                            {
                                for (int c = 0; c < 3; c++)
                                {
                                    float fval = img_background[background_size * c + mappos] / 255.0;
                                    rgb_output.channel(c)[y * rgb_output.width() + x] = rgb_output.clamp(fval * 65535);
                                }
                            }
                            else
                            {
                                for (int c = 0; c < 3; c++)
                                {
                                    float mval = img_background[background_size * c + mappos] / 255.0;
                                    float fval = mval * 0.4 + val * 0.6;
                                    rgb_output.channel(c)[y * rgb_output.width() + x] = rgb_output.clamp(fval * 65535);
                                }
                            }
                        }
                    }
                }
                else
                {
                    rgb_output.channel(0)[y * rgb_output.width() + x] = 0;
                    rgb_output.channel(1)[y * rgb_output.width() + x] = 0;
                    rgb_output.channel(2)[y * rgb_output.width() + x] = 0;
                }
            }

            // set the progress bar accordingly
            if (progress != nullptr)
                *progress = double(x) / double(rgb_output.width());
        }

        delete[] channelVals;

        return rgb_output;
    }
}