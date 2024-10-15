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
#include <numeric>
#include <vector>
#define DEFINE_COMPOSITE_UTILS 1
#include "common/image/composite.h"
#include "common/image/hue_saturation.h"
#include "logger.h"


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

		//image::Image output(f.img_depth, f.maxWidth, f.maxHeight, 4);

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
        	float cfg_thresold = vars["thresold"];
        	bool cfg_blend = vars["blend"];
        	bool cfg_invert = vars["invert"];

		size_t background_size = img_background.width() * img_background.height();
		auto &ch_equal = inputChannels[0];
		image::equalize(ch_equal);
		float val = 0;

        	float proj_x_scale = 1.0;
        	if (proj_cfg.contains("width"))
        	    proj_x_scale = proj_cfg["width"].get<double>() / double(rgb_output.width());

        	float proj_y_scale = 1.0;
        	if (proj_cfg.contains("height"))
        	    proj_y_scale = proj_cfg["height"].get<double>() / double(rgb_output.height());



                for (size_t x = 0; x < rgb_output.width(); x++)
                {
                    for (size_t y = 0; y < rgb_output.height(); y++)
                    {
			    if (!projFunc->get_position(x * proj_x_scale, y * proj_y_scale, coords))
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


				    for (int c = 0; c < 3; c++)
				    {
					    float fval = img_background.getf(background_size * c + mappos);
					    rgb_output.setf(c, y * rgb_output.width() + x, rgb_output.clampf(fval));
				    }




                                    //int t4_21 = img_pro->get_calibrated_value(23, x, y, true);
                                    //int t4_22 = img_pro->get_calibrated_value(24, x, y, true);
                                    //int t11 = img_pro->get_calibrated_value(33, x, y, true);
                                    //int t12 = img_pro->get_calibrated_value(34, x, y, true);




			            //if (t12 < 0)
			            //{
			            //	t12 = 0;
			            //}

			            //if (t4_22 < 0)
			            //{
			            //	t4_22 = 0;
			            //}
			            //if (t4_21 < 0)
			            //{
			            //	t4_21 = 0;
			            //}
			            //if (t11 < 0)
			            //{
			            //	t11 = 0;
			            //}

			            //if (t12 >= 265 && t4_22 > 310 && t4_22 - t11 > 15)
			            //{
			            //	output.set(0, y * output.width() + x, 1);
			            //	output.set(1, y * output.width() + x, 0);
			            //	output.set(2, y * output.width() + x, 0);
			            //	output.set(3, y * output.width() + x, 1);
			            //}
			            //else
			            //{
			            //	output.set(0, y * output.width() + x, 0);
			            //	output.set(1, y * output.width() + x, 0);
			            //	output.set(2, y * output.width() + x, 0);
			            //	output.set(3, y * output.width() + x, 0);
			            //}
			    }
                    }

                    // set the progress bar accordingly
                    if (progress != nullptr)
                        *progress = double(x) / double(rgb_output.width());
                }

	//delete[] channelVals;


	//image::HueSaturation hueTunning;
	//hueTunning.hue[image::HUE_RANGE_YELLOW] = -45.0 / 180.0;
        //hueTunning.hue[image::HUE_RANGE_RED] = 90.0 / 180.0;
        //hueTunning.overlap = 100.0 / 100.0;
        //image::hue_saturation(output, hueTunning);

        return rgb_output;
    }
}
