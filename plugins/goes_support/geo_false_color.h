#pragma once

#include "products/image_products.h"
#include "resources.h"
#define DEFINE_COMPOSITE_UTILS 1
#include "common/image/composite.h"
#include "core/exception.h"
#include "common/image/io.h"

namespace goes
{
    image::Image goesFalseColorCompositor(satdump::ImageProducts *img_pro,
                                           std::vector<image::Image> &inputChannels,
                                           std::vector<std::string> channelNumbers,
                                           std::string cpp_id,
                                           nlohmann::json vars,
                                           nlohmann::json offsets_cfg,
                                           std::vector<double> *final_timestamps = nullptr,
                                           float *progress = nullptr)
    {
        image::compo_cfg_t f = image::get_compo_cfg(inputChannels, channelNumbers, offsets_cfg);

        std::string lut_path = vars.contains("lut") ? vars["lut"].get<std::string>() : std::string("goes/abi/wxstar/lut.png");

        // Load the lut and curve
        image::Image img_lut;
        image::Image img_curve;
        image::load_png(img_lut, resources::getResourcePath(lut_path));
        image::load_png(img_curve, resources::getResourcePath("goes/abi/wxstar/ch2_curve.png"));
        size_t lut_size = img_lut.height() * img_lut.width();
        size_t lut_width = img_lut.width();

        if (img_lut.width() != 256 || img_lut.height() != 256 || img_lut.channels() < 3)
            logger->error("Lut " + lut_path + " did not load!");

        if (f.img_depth != 8)
            throw satdump_exception("Geo False Color MUST be 8-bits at the moment."); // TODOIMG

        // return 3 channels, RGB
        image::Image output(f.img_depth, f.maxWidth, f.maxHeight, 3);

        int *channelVals = new int[inputChannels.size()];

        for (size_t x = 0; x < output.width(); x++)
        {
            for (size_t y = 0; y < output.height(); y++)
            {
                // get channels from satdump.json
                image::get_channel_vals_raw(channelVals, inputChannels, f, y, x);
                int lut_pos = (img_curve.get(channelVals[0]) * lut_width) + (channelVals[1]);

                // return RGB 0=R 1=G 2=B
                output.set(0, x, y, img_lut.get(0, lut_pos));
                output.set(1, x, y, img_lut.get(1, lut_pos));
                output.set(2, x, y, img_lut.get(2, lut_pos));
            }

            // set the progress bar accordingly
            if (progress != nullptr)
                *progress = double(x) / double(output.width());
        }

        delete[] channelVals;

        return output;
    }
}