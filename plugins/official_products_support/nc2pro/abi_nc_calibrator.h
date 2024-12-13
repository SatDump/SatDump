#pragma once

#include "products/image_products.h"
#include "nlohmann/json.hpp"
#include "common/calibration.h"
#include "common/projection/sat_proj/sat_proj.h"
#include "common/projection/reprojector.h"

namespace nc2pro
{
    class ABINcCalibrator : public satdump::ImageProducts::CalibratorBase
    {
    private:
        std::map<std::string, double> calibration_scale;
        std::map<std::string, double> calibration_offset;
        std::map<std::string, double> calibration_kappa;
        std::vector<std::string> channel_lut;

    public:
        ABINcCalibrator(nlohmann::json calib, satdump::ImageProducts* products) : satdump::ImageProducts::CalibratorBase(calib, products)
        {
            size_t num_channels = products->images.size();
            for (size_t i = 0; i < num_channels; i++)
            {
                std::string channel_name = products->images[i].channel_name;
                calibration_scale[channel_name] = calib["vars"]["scale"][i];
                calibration_offset[channel_name] = calib["vars"]["offset"][i];
                calibration_kappa[channel_name] = calib["vars"]["kappa"][i];
                channel_lut.emplace_back(channel_name);
            }
        }

        void init()
        {
        }

        double compute(int channel, int /* pos_x */, int /* pos_y */, int px_val)
        {
            if (px_val == 0)
                return CALIBRATION_INVALID_VALUE;

            std::string channel_str = channel_lut[channel];

            if (calibration_offset[channel_str] == 0 || calibration_scale[channel_str] == 0)
                return CALIBRATION_INVALID_VALUE;

            double rad = calibration_offset[channel_str] + px_val * calibration_scale[channel_str];

            if (calibration_kappa[channel_str] > 0)
                return rad * calibration_kappa[channel_str];
            else
                return rad;
        }
    };
}
