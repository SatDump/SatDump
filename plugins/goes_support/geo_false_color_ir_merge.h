#pragma once

#include "products/image_products.h"
#include "resources.h"
#define DEFINE_COMPOSITE_UTILS 1
#include "common/image/composite.h"

#include "libs/predict/predict.h"
#include "common/projection/sat_proj/sat_proj.h"

#include "common/projection/projs/equirectangular.h"

namespace goes
{
    image::Image<uint16_t> goesFalseColorIRMergeCompositor(satdump::ImageProducts *img_pro,
                                                           std::vector<image::Image<uint16_t>> &inputChannels,
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
        image::Image<uint8_t> img_lut;
        image::Image<uint8_t> img_curve;
        img_lut.load_png(resources::getResourcePath(lut_path));
        img_curve.load_png(resources::getResourcePath("goes/abi/wxstar/ch2_curve.png"));
        size_t lut_size = img_lut.height() * img_lut.width();
        size_t lut_width = img_lut.width();

        if (img_lut.width() != 256 || img_lut.height() != 256 || img_lut.channels() < 3)
            logger->error("Lut " + lut_path + " did not load!");

        image::Image<uint8_t> img_nightmap;
        img_nightmap.load_img(resources::getResourcePath("maps/nasa_night.jpg"));
        geodetic::projection::EquirectangularProjection equp;
        equp.init(img_nightmap.width(), img_nightmap.height(), -180, 90, 180, -90);
        int map_x, map_y;

        // return 3 channels, RGB
        image::Image<uint16_t> output(f.maxWidth, f.maxHeight, 3);

        uint16_t *channelVals = new uint16_t[inputChannels.size()];

        double timestamp = 0;
        if (img_pro->has_timestamps && img_pro->get_timestamps().size() == 1 && img_pro->timestamp_type == satdump::ImageProducts::TIMESTAMP_SINGLE_IMAGE)
            timestamp = img_pro->get_timestamps()[0];
        else if (img_pro->has_product_timestamp()) // Maybe to remove?
            timestamp = img_pro->get_product_timestamp();
        predict_julian_date_t julian_date = predict_to_julian(timestamp);

        img_pro->get_product_timestamp();
        predict_observer_t *observer = predict_create_observer("Main", 0, 0, 0);
        predict_observation obs;

        auto projFunc = satdump::get_sat_proj(img_pro->get_proj_cfg(), img_pro->get_tle(), *final_timestamps, true);
        geodetic::geodetic_coords_t coords;

        for (size_t x = 0; x < output.width(); x++)
        {
            for (size_t y = 0; y < output.height(); y++)
            {
                // get channels from satdump.json
                image::get_channel_vals_raw(channelVals, inputChannels, f, y, x);
                int lut_pos = (img_curve[channelVals[0] >> 8] * lut_width) + (channelVals[1] >> 8);

                if (!projFunc->get_position(x, y, coords))
                {
                    observer->longitude = coords.lon * DEG2RAD;
                    observer->latitude = coords.lat * DEG2RAD;

                    predict_observe_sun(observer, julian_date, &obs);

                    float val = ((obs.elevation * RAD_TO_DEG) / 10.0);
                    if (val < 0)
                        val = 0;
                    if (val > 1)
                        val = 1;

                    equp.forward(coords.lon, coords.lat, map_x, map_y);

                    float mcir_v = float(channelVals[1]) / 65535.0;
                    mcir_v = (mcir_v - 0.15) * 3;
                    if (mcir_v > 1)
                        mcir_v = 1;
                    if (mcir_v < 0)
                        mcir_v = 0;

                    uint16_t mcir_val0 = 0, mcir_val1 = 0, mcir_val2 = 0;
                    if (map_x != -1 && map_y != -1)
                    {
                        mcir_val0 = channelVals[1] * mcir_v + (img_nightmap.channel(0)[map_y * img_nightmap.width() + map_x] << 8) * (1.0 - mcir_v);
                        mcir_val1 = channelVals[1] * mcir_v + (img_nightmap.channel(1)[map_y * img_nightmap.width() + map_x] << 8) * (1.0 - mcir_v);
                        mcir_val2 = channelVals[1] * mcir_v + (img_nightmap.channel(2)[map_y * img_nightmap.width() + map_x] << 8) * (1.0 - mcir_v);
                    }

                    output.channel(0)[y * output.width() + x] = (img_lut[0 * lut_size + lut_pos] << 8) * val + mcir_val0 * (1.0 - val);
                    output.channel(1)[y * output.width() + x] = (img_lut[1 * lut_size + lut_pos] << 8) * val + mcir_val1 * (1.0 - val);
                    output.channel(2)[y * output.width() + x] = (img_lut[2 * lut_size + lut_pos] << 8) * val + mcir_val2 * (1.0 - val);
                }
                else
                {
                    output.channel(0)[y * output.width() + x] = 0;
                    output.channel(1)[y * output.width() + x] = 0;
                    output.channel(2)[y * output.width() + x] = 0;
                }

                // return RGB 0=R 1=G 2=B
                //  output.channel(0)[y * output.width() + x] = img_lut[0 * lut_size + lut_pos] << 8;
                //  output.channel(1)[y * output.width() + x] = img_lut[1 * lut_size + lut_pos] << 8;
                //  output.channel(2)[y * output.width() + x] = img_lut[2 * lut_size + lut_pos] << 8;
            }

            // set the progress bar accordingly
            if (progress != nullptr)
                *progress = double(x) / double(output.width());
        }

        delete[] channelVals;

        return output;
    }
}