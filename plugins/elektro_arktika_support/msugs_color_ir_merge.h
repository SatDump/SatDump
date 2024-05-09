#pragma once

#include "products/image_products.h"
#include "resources.h"
#define DEFINE_COMPOSITE_UTILS 1
#include "common/image/composite.h"

#include "libs/predict/predict.h"
#include "common/projection/sat_proj/sat_proj.h"

#include "common/projection/projs/equirectangular.h"
#include "common/image/io.h"

#include "core/exception.h"

namespace elektro
{
    image::Image msuGsFalseColorIRMergeCompositor(satdump::ImageProducts *img_pro,
                                                  std::vector<image::Image> &inputChannels,
                                                  std::vector<std::string> channelNumbers,
                                                  std::string cpp_id,
                                                  nlohmann::json vars,
                                                  nlohmann::json offsets_cfg,
                                                  std::vector<double> *final_timestamps = nullptr,
                                                  float *progress = nullptr)
    {
        image::compo_cfg_t f = image::get_compo_cfg(inputChannels, channelNumbers, offsets_cfg);

        image::Image img_nightmap;
        image::load_img(img_nightmap, resources::getResourcePath("maps/nasa_night.jpg"));
        geodetic::projection::EquirectangularProjection equp;
        equp.init(img_nightmap.width(), img_nightmap.height(), -180, 90, 180, -90);
        int map_x, map_y;

        if (img_nightmap.depth() != f.img_depth)
            img_nightmap = img_nightmap.to_depth(f.img_depth);

        // return 3 channels, RGB
        image::Image output(f.img_depth, f.maxWidth, f.maxHeight, 3);

        int *channelVals = new int[inputChannels.size()];

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

                    channelVals[3] = 65535 - channelVals[3];
                    float mcir_v = float(channelVals[3]) / 65535.0;
                    if (mcir_v > 1)
                        mcir_v = 1;
                    if (mcir_v < 0)
                        mcir_v = 0;

                    int mcir_val0 = 0, mcir_val1 = 0, mcir_val2 = 0;
                    if (map_x != -1 && map_y != -1)
                    {
                        mcir_val0 = channelVals[3] * mcir_v + img_nightmap.get(0, map_y * img_nightmap.width() + map_x) * (1.0 - mcir_v);
                        mcir_val1 = channelVals[3] * mcir_v + img_nightmap.get(1, map_y * img_nightmap.width() + map_x) * (1.0 - mcir_v);
                        mcir_val2 = channelVals[3] * mcir_v + img_nightmap.get(2, map_y * img_nightmap.width() + map_x) * (1.0 - mcir_v);
                    }

                    output.set(0, y * output.width() + x, channelVals[0] * val + mcir_val0 * (1.0 - val));
                    output.set(1, y * output.width() + x, channelVals[1] * val + mcir_val1 * (1.0 - val));
                    output.set(2, y * output.width() + x, channelVals[2] * val + mcir_val2 * (1.0 - val));
                }
                else
                {
                    output.set(0, y * output.width() + x, 0);
                    output.set(1, y * output.width() + x, 0);
                    output.set(2, y * output.width() + x, 0);
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