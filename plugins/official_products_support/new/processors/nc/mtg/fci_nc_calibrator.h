#pragma once

#include "products/image/image_calibrator.h"
#include "nlohmann/json.hpp"
#include "common/calibration.h"

namespace nc2pro
{
    class FCINcCalibrator : public satdump::products::ImageCalibrator
    {
    private:
        double calibration_scale[16];
        double calibration_offset[16];
        int channel_lut[16];

    public:
        FCINcCalibrator(satdump::products::ImageProduct *p, nlohmann::json c)
            : satdump::products::ImageCalibrator(p, c)
        {
            for (int i = 0; i < 16; i++)
            {
                calibration_scale[i] = c["vars"]["scale"][i];
                calibration_offset[i] = c["vars"]["offset"][i];
            }

            for (size_t i = 0; i < p->images.size(); i++)
                channel_lut[i] = std::stoi(p->images[i].channel_name) - 1;
        }

        void init()
        {
        }

        double compute(int channel, int pos_x, int pos_y, uint32_t px_val)
        {
            if (px_val == 0)
                return CALIBRATION_INVALID_VALUE;

            channel = channel_lut[channel];

            double physical_units = calibration_offset[channel] + px_val * calibration_scale[channel];

            if (calibration_offset[channel] == 0 || calibration_scale[channel] == 0)
                return CALIBRATION_INVALID_VALUE;

            return physical_units;
        }
    };
}
