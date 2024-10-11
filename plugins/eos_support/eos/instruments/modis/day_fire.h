#pragma once

#include "common/image/image.h"
#include "products/image_products.h"
#include "resources.h"
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

		image::Image output(f.img_depth, f.maxWidth, f.maxHeight, 4);

		//int *channelVals = new int[inputChannels.size()];

		image::Image tmp(f.img_depth, f.maxWidth, f.maxHeight, 4);

                for (size_t x = 0; x < output.width(); x++)
                {
                    for (size_t y = 0; y < output.height(); y++)
                    {

			float sum = 0.0;
			float mean = 0.0;
			float stddev = 0.0;

			// x - 1, y - 1 + x, y - 1 + x + 1, y - 1
			// x - 1, y + x, y + x + 1, y
			// x - 1, y + 1 + x + 1, y + x + 1, y + 1
			//  ________________________
			// | -1, -1 | 0, -1 | 1, -1 |
			// |________________________|
			// | -1, 0  | 0, 0  | 1, 0  |
			// |________________________|
			// | -1, 1  | 1, 0  | 1, 1  |
			// |________________________|


			float t4_21_array[9] = { 0 };

			t4_21_array[0] += img_pro->get_calibrated_value(23, x - 1, y - 1, true);
			t4_21_array[1] += img_pro->get_calibrated_value(23, x, y - 1, true);
			t4_21_array[2] += img_pro->get_calibrated_value(23, x + 1, y - 1, true);
			t4_21_array[3] += img_pro->get_calibrated_value(23, x - 1, y, true);
			t4_21_array[4] += img_pro->get_calibrated_value(23, x, y, true);
			t4_21_array[5] += img_pro->get_calibrated_value(23, x + 1, y, true);
			t4_21_array[6] += img_pro->get_calibrated_value(23, x - 1, y + 1, true);
			t4_21_array[7] += img_pro->get_calibrated_value(23, x, y + 1, true);
			t4_21_array[8] += img_pro->get_calibrated_value(23, x + 1, y + 1, true);

			for (int i = 0; i < 9; ++i)
				sum += t4_21_array[i];

			mean = ((float)sum) / 9;

			for (int i = 0; i < 9; ++i)
				stddev += sqrt(pow(t4_21_array[i] - mean, 2) / 9);

			printf("Mean : %f Standard deviation : %f\n", mean, stddev);


			int ch1 = img_pro->get_calibrated_value(0, x, y, false);
			int ch2 = img_pro->get_calibrated_value(1, x, y, false);

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

			if (t4_22 > 310 && t4_21 - t11 > 25)
			{
				//printf("T4_21 : %d", t4_21);
				output.set(0, y * output.width() + x, 1);
				output.set(1, y * output.width() + x, 0);
				output.set(2, y * output.width() + x, 0);
				output.set(3, y * output.width() + x, 1);
			}
			else
			{
				output.set(0, y * output.width() + x, 0);
				output.set(1, y * output.width() + x, 0);
				output.set(2, y * output.width() + x, 0);
				output.set(3, y * output.width() + x, 0);
			}

			//int t4_21 = channelVals[0];
			//int t4_22 = channelVals[1];
			//int t11 = channelVals[2];

			//printf("T4_21 : %d T4_22 : %d T11 : %d\n", t4_21, t4_22, t11);
			//logger->info("T4_21 : %d", t4_21);

                        //output.set(0, y * output.width() + x, t4_21);
                        //output.set(1, y * output.width() + x, t4_22);
                        //output.set(2, y * output.width() + x, t11);

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


	//image::HueSaturation hueTunning;
	//hueTunning.hue[image::HUE_RANGE_YELLOW] = -45.0 / 180.0;
        //hueTunning.hue[image::HUE_RANGE_RED] = 90.0 / 180.0;
        //hueTunning.overlap = 100.0 / 100.0;
        //image::hue_saturation(output, hueTunning);

        return output;
    }
}
