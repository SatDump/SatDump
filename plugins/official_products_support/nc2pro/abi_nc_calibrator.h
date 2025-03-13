#pragma once

#include "products2/image/image_calibrator.h"
#include "nlohmann/json.hpp"

namespace nc2pro
{
    class ABINcCalibrator : public satdump::products::ImageCalibrator
    {
    private:
        std::map<std::string, double> calibration_scale;
        std::map<std::string, double> calibration_offset;
        std::map<std::string, double> calibration_kappa;
        std::vector<std::string> channel_lut;

    public:
        ABINcCalibrator(satdump::products::ImageProduct *p, nlohmann::json c)
            : satdump::products::ImageCalibrator(p, c)
        {
            size_t num_channels = p->images.size();
            for (size_t i = 0; i < num_channels; i++)
            {
                std::string channel_name = p->images[i].channel_name;
                calibration_scale[channel_name] = c["vars"]["scale"][i];
                calibration_offset[channel_name] = c["vars"]["offset"][i];
                calibration_kappa[channel_name] = c["vars"]["kappa"][i];
                channel_lut.emplace_back(channel_name);
            }
        }

        void init()
        {
        }

        double compute(int channel, int /* pos_x */, int /* pos_y */, uint32_t px_val)
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
