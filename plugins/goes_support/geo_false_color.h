#pragma once

#include "products/image_products.h"
#include "resources.h"
#define DEFINE_COMPOSITE_UTILS 1
#include "common/image/composite.h"

namespace goes
{
    image::Image<uint16_t> goesFalseColorCompositor(satdump::ImageProducts *img_pro,
                                                    std::vector<image::Image<uint16_t>> &inputChannels,
                                                    std::vector<std::string> channelNumbers,
                                                    std::string cpp_id,
                                                    nlohmann::json vars,
                                                    nlohmann::json offsets_cfg,
                                                    std::vector<double> *final_timestamps = nullptr,
                                                    float *progress = nullptr)
    {
        image::compo_cfg_t f = image::get_compo_cfg(inputChannels, /*channelNumbers, */ offsets_cfg);

        // Load the lut and curve
        image::Image<uint8_t> img_lut;
        image::Image<uint8_t> img_curve;
        img_lut.load_png(resources::getResourcePath("goes/abi/wxstar/lut.png"));
        img_curve.load_png(resources::getResourcePath("goes/abi/wxstar/ch2_curve.png"));
        size_t lut_size = img_lut.height() * img_lut.width();
        size_t lut_width = img_lut.width();

        // return 3 channels, RGB
        image::Image<uint16_t> output(f.maxWidth, f.maxHeight, 3);

        uint16_t *channelVals = new uint16_t[inputChannels.size()];

        for (size_t x = 0; x < output.width(); x++)
        {
            for (size_t y = 0; y < output.height(); y++)
            {
                // get channels from satdump.json
                image::get_channel_vals_raw(channelVals, inputChannels, channelNumbers, f, y, x);
                int lut_pos = (img_curve[channelVals[0] >> 8] * lut_width) + (channelVals[1] >> 8);

                // return RGB 0=R 1=G 2=B
                output.channel(0)[y * output.width() + x] = img_lut[0 * lut_size + lut_pos] << 8;
                output.channel(1)[y * output.width() + x] = img_lut[1 * lut_size + lut_pos] << 8;
                output.channel(2)[y * output.width() + x] = img_lut[2 * lut_size + lut_pos] << 8;
            }

            // set the progress bar accordingly
            if (progress != nullptr)
                *progress = double(x) / double(output.width());
        }

        delete channelVals;

        return output;
    }
}