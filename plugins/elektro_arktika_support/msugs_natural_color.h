#pragma once

#include "products/image_products.h"
#include "resources.h"
#define DEFINE_COMPOSITE_UTILS 1
#include "common/image/composite.h"
#include "common/image/hue_saturation.h"

namespace elektro
{
    image::Image<uint16_t> msuGsNaturalColorCompositor(satdump::ImageProducts *img_pro,
                                                       std::vector<image::Image<uint16_t>> &inputChannels,
                                                       std::vector<std::string> channelNumbers,
                                                       std::string cpp_id,
                                                       nlohmann::json vars,
                                                       nlohmann::json offsets_cfg,
                                                       std::vector<double> *final_timestamps = nullptr,
                                                       float *progress = nullptr)
    {
        image::compo_cfg_t f = image::get_compo_cfg(inputChannels, channelNumbers, offsets_cfg);

        // return 3 channels, RGB. Generate 321
        image::Image<uint16_t> output(f.maxWidth, f.maxHeight, 3);

        uint16_t *channelVals = new uint16_t[inputChannels.size()];

        for (size_t x = 0; x < output.width(); x++)
        {
            for (size_t y = 0; y < output.height(); y++)
            {
                // get channels from satdump.json
                image::get_channel_vals_raw(channelVals, inputChannels, f, y, x);

                // return RGB 0=R 1=G 2=B
                output.channel(0)[y * output.width() + x] = channelVals[0];
                output.channel(1)[y * output.width() + x] = channelVals[1];
                output.channel(2)[y * output.width() + x] = channelVals[2];
            }

            // set the progress bar accordingly
            if (progress != nullptr)
                *progress = double(x) / double(output.width());
        }

        delete[] channelVals;

        // Switch to nat color
        image::HueSaturation hueTuning;
        hueTuning.hue[image::HUE_RANGE_YELLOW] = -45.0 / 180.0;
        hueTuning.hue[image::HUE_RANGE_RED] = 90.0 / 180.0;
        hueTuning.overlap = 100.0 / 100.0;
        image::hue_saturation(output, hueTuning);

        return output;
    }
}