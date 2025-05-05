#pragma once

#include "products/image_products.h"
#include "nlohmann/json.hpp"
#include "common/calibration.h"
#include "common/projection/sat_proj/sat_proj.h"
#include "common/projection/reprojector.h"

namespace nc2pro
{
    class FCINcCalibrator : public satdump::ImageProducts::CalibratorBase
    {
    private:
        double calibration_scale[16];
        double calibration_offset[16];
        int channel_lut[16];

    public:
        FCINcCalibrator(nlohmann::json calib, satdump::ImageProducts *products) : satdump::ImageProducts::CalibratorBase(calib, products)
        {
            for (int i = 0; i < 16; i++)
            {
                calibration_scale[i] = calib["vars"]["scale"][i];
                calibration_offset[i] = calib["vars"]["offset"][i];
            }

            for (size_t i = 0; i < products->images.size(); i++)
                channel_lut[i] = std::stoi(products->images[i].channel_name) - 1;
        }

        void init()
        {
        }

        double compute(int channel, int pos_x, int pos_y, int px_val)
        {
            if (px_val == 0)
                return CALIBRATION_INVALID_VALUE;

            channel = channel_lut[channel];

            double physical_units = calibration_offset[channel] + px_val * calibration_scale[channel];

            if (calibration_offset[channel] == 0 || calibration_scale[channel] == 0)
                return CALIBRATION_INVALID_VALUE;

            /* if (channel == 0 || channel == 1 || channel == 2 || channel == 11)
             {
                 if (!projs[channel]->get_position(pos_x, pos_y, coords))
                 {
                     coords.toDegs();
                     logger->trace("%f %f", coords.lon, coords.lat);
                     return radiance_to_reflectance(channel + 1, physical_units, 2024, 7, 6, 11, 45, coords.lat, coords.lon);
                 }
                 else
                 {
                     return CALIBRATION_INVALID_VALUE;
                 }
             }
             else*/
            return physical_units;
        }
    };
}
