#pragma once

#include "common/image/image.h"
#include "products/image_products.h"
#include "resources.h"
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

		image::Image output(f.img_depth, f.maxWidth, f.maxHeight, 3);

		//int *channelVals = new int[inputChannels.size()];

                for (size_t x = 0; x < output.width(); x++)
                {
                    for (size_t y = 0; y < output.height(); y++)
                    {
                        // get channels from satdump.json
                        int t4_21 = img_pro->get_calibrated_value(23, x, y, true);
                        int t4_22 = img_pro->get_calibrated_value(24, x, y, true);
                        int t11 = img_pro->get_calibrated_value(33, x, y, true);


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


			//int t4_21 = channelVals[0];
			//int t4_22 = channelVals[1];
			//int t11 = channelVals[2];

			//printf("T4_21 : %d T4_22 : %d T11 : %d\n", t4_21, t4_22, t11);
			//logger->info("T4_21 : %d", t4_21);

                        output.set(0, y * output.width() + x, t4_21);
                        output.set(1, y * output.width() + x, t4_22);
                        output.set(2, y * output.width() + x, t11);

                        // return RGB 0=R 1=G 2=B
                        //output.set(0, y * output.width() + x, channelVals[0]);
                        //output.set(1, y * output.width() + x, channelVals[1]);
                        //output.set(2, y * output.width() + x, channelVals[2]);
                    }

                    // set the progress bar accordingly
                    if (progress != nullptr)
                        *progress = double(x) / double(output.width());
                }

	//delete[] channelVals;


	image::HueSaturation hueTunning;
	hueTunning.hue[image::HUE_RANGE_YELLOW] = -45.0 / 180.0;
        hueTunning.hue[image::HUE_RANGE_RED] = 90.0 / 180.0;
        hueTunning.overlap = 100.0 / 100.0;
        image::hue_saturation(output, hueTunning);

        return output;
    }
}
