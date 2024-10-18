#pragma once

#include "common/geodetic/geodetic_coordinates.h"
#include "common/image/image.h"
#include "common/image/processing.h"
#include "common/image/io.h"
#include "common/projection/projs/equirectangular.h"
#include "common/projection/sat_proj/sat_proj.h"
#include "products/image_products.h"
#include "resources.h"
#include <cstdlib>
#include <vector>
#define DEFINE_COMPOSITE_UTILS 1
#include "common/image/composite.h"


namespace modis
{
	image::Image dayFireCompositor(satdump::ImageProducts *img_pro,
			std::vector<image::Image> &inputChannels,
			std::vector<std::string> channelNumbers,
			std::string cpp_id,
			nlohmann::json vars,
			nlohmann::json offsets_cfg,
			std::vector<double> *final_timestamps = nullptr,
			float *progress = nullptr)
	{
		image::compo_cfg_t f = image::get_compo_cfg(inputChannels, channelNumbers, offsets_cfg);

		image::Image output(f.img_depth, f.maxWidth, f.maxHeight, 3);

		image::Image img_background;
		image::load_img(img_background, resources::getResourcePath("maps/nasa_hd.jpg"));
		geodetic::projection::EquirectangularProjection equp;
		equp.init(img_background.width(), img_background.height(), -180, 90, 180, -90);
		int map_x, map_y;

		image::Image rgb_output(f.img_depth, f.maxWidth, f.maxHeight, 3);

		geodetic::geodetic_coords_t coords;
		auto proj_cfg = img_pro->get_proj_cfg();
		auto projFunc = satdump::get_sat_proj(proj_cfg, img_pro->get_tle(), *final_timestamps, true);

		float cfg_offset = vars["minoffset"];
		float cfg_scalar = vars["scalar"];

		size_t background_size = img_background.width() * img_background.height();
		auto &ch_equal = inputChannels[0];
		image::equalize(ch_equal);
		float val = 0;

		float proj_scale = 1.0;
		if (proj_cfg.contains("image_width"))
			proj_scale = proj_cfg["image_width"].get<double>() / double(rgb_output.width());


                for (size_t x = 0; x < rgb_output.width(); x++)
                {
                    for (size_t y = 0; y < rgb_output.height(); y++)
                    {
			    if (!projFunc->get_position(x * proj_scale, y * proj_scale, coords))
			    {
				    equp.forward(coords.lon, coords.lat, map_x, map_y);

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

				    for (int c = 0; c < 3; c++)
				    {
					    float mval = img_background.getf(background_size * c + mappos);
					    rgb_output.setf(c, y * rgb_output.width() + x, rgb_output.clampf(mval));
				    }

			    }
		    }
		    // set first progerss bar
		    if (progress != nullptr)
                        *progress = double(x) / double(rgb_output.width());
		}

                for (size_t x = 0; x < rgb_output.width(); x++)
                {
                    for (size_t y = 0; y < rgb_output.height(); y++)
                    {

			    int t4_21 = img_pro->get_calibrated_value(23, x, y, true);
			    int t4_22 = img_pro->get_calibrated_value(24, x, y, true);
                            int t11 = img_pro->get_calibrated_value(33, x, y, true);
                            int t12 = img_pro->get_calibrated_value(34, x, y, true);

			    if (t12 < 0)
                            {
                            	t12 = 0;
                            }
                            
                            if (t4_22 < 0)
                            {
                            	t4_22 = 0;
                            }
                            if (t4_21 < 0)
                            {
                            	t4_21 = 0;
                            }
                            if (t11 < 0)
                            {
                            	t11 = 0;
                            }

			    //if (t12 >= 265 && t4_22 > 310 && t4_22 - t11 > 15)
                            
                            if (t12 >= 265 && t4_22 > 310 && t4_22 - t11 > 15)
                            {
                                    //rgb_output.draw_circle(x, y, 3, {1.0,0,0}, true);
				    rgb_output.draw_rectangle(x - 2, y + 2, x + 2, y - 2, {1,0,0}, true);
                            }
                    }
                    // set second progress bar
                    if (progress != nullptr)
                        *progress = double(x) / double(rgb_output.width());
                }

        return rgb_output;
    }
}