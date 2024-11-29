#pragma once

#include "products/image_products.h"
#include "resources.h"
#define DEFINE_COMPOSITE_UTILS 1
#include "common/image/composite.h"

#include "common/projection/sat_proj/sat_proj.h"
#include "common/projection/projs/equirectangular.h"
#include "common/image/io.h"
#include "common/image/processing.h"

namespace cpp_compos
{
    image::Image underlay_with_clouds(satdump::ImageProducts *img_pro,
                                      std::vector<image::Image> &inputChannels,
                                      std::vector<std::string> channelNumbers,
                                      std::string /* cpp_id */,
                                      nlohmann::json vars,
                                      nlohmann::json offsets_cfg,
                                      std::vector<double> *final_timestamps = nullptr,
                                      float *progress = nullptr)
    {
        if (!img_pro->has_proj_cfg())
        {
            logger->error("This composite requires projection info");
            return image::Image();
        }

        image::compo_cfg_t f = image::get_compo_cfg(inputChannels, channelNumbers, offsets_cfg);

        image::Image img_background;
        image::load_img(img_background, resources::getResourcePath("maps/nasa_hd.jpg"));
        geodetic::projection::EquirectangularProjection equp;
        equp.init(img_background.width(), img_background.height(), -180, 90, 180, -90);
        int map_x, map_y;

        // return 3 channels, RGB
        image::Image rgb_output(f.img_depth, f.maxWidth, f.maxHeight, 3);

        geodetic::geodetic_coords_t coords;
        auto proj_cfg = img_pro->get_proj_cfg();
        auto projFunc = satdump::get_sat_proj(proj_cfg, img_pro->get_tle(), *final_timestamps, true);

        float cfg_offset = vars["minoffset"];
        float cfg_scalar = vars["scalar"];
        float cfg_thresold = vars["thresold"];
        bool cfg_blend = vars["blend"];
        bool cfg_invert = vars["invert"];

        size_t background_size = img_background.width() * img_background.height();
        auto &ch_equal = inputChannels[0];
        image::equalize(ch_equal);
        float val = 0;

        float proj_scale = 1.0f;
        if (proj_cfg.contains("image_width"))
            proj_scale = proj_cfg["image_width"].get<double>() / double(rgb_output.width());


        for (size_t x = 0; x < rgb_output.width(); x++)
        {
            for (size_t y = 0; y < rgb_output.height(); y++)
            {
                if (!projFunc->get_position(x * proj_scale, y * proj_scale, coords))
                {
                    equp.forward(coords.lon, coords.lat, map_x, map_y);

                    if (cfg_invert)
                        val = 1.0 - (ch_equal.getf((y * rgb_output.width()) + x) - cfg_offset) * cfg_scalar;
                    else
                        val = (ch_equal.getf((y * rgb_output.width()) + x) - cfg_offset) * cfg_scalar;

                    if (val > 1)
                        val = 1;
                    if (val < 0)
                        val = 0;

                    int mappos = map_y * img_background.width() + map_x;

                    if (mappos >= (int)background_size)
                        mappos = background_size - 1;

                    if (mappos < 0)
                        mappos = 0;

                    if (cfg_blend == 1)
                    {
                        for (int c = 0; c < 3; c++)
                        {
                            float mval = img_background.getf(background_size * c + mappos);
                            float fval = mval * (1.0 - val) + val * val;
                            rgb_output.setf(c, y * rgb_output.width() + x, rgb_output.clampf(fval));
                        }
                    }
                    else
                    {
                        if (cfg_thresold == 0)
                        {
                            for (int c = 0; c < 3; c++)
                            {
                                float mval = img_background.getf(background_size * c + mappos);
                                float fval = mval * 0.4 + val * 0.6;
                                rgb_output.setf(c, y * rgb_output.width() + x, rgb_output.clampf(fval));
                            }
                        }
                        else
                        {
                            if (val < cfg_thresold)
                            {
                                for (int c = 0; c < 3; c++)
                                {
                                    float fval = img_background.getf(background_size * c + mappos);
                                    rgb_output.setf(c, y * rgb_output.width() + x, rgb_output.clampf(fval));
                                }
                            }
                            else
                            {
                                for (int c = 0; c < 3; c++)
                                {
                                    float mval = img_background.getf(background_size * c + mappos);
                                    float fval = mval * 0.4 + val * 0.6;
                                    rgb_output.setf(c, y * rgb_output.width() + x, rgb_output.clampf(fval));
                                }
                            }
                        }
                    }
                }
                else
                {
                    rgb_output.setf(0, y * rgb_output.width() + x, 0);
                    rgb_output.setf(1, y * rgb_output.width() + x, 0);
                    rgb_output.setf(2, y * rgb_output.width() + x, 0);
                }
            }

            // set the progress bar accordingly
            if (progress != nullptr)
                *progress = double(x) / double(rgb_output.width());
        }

        return rgb_output;
    }
}