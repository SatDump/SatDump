#pragma once

#include "products/image/image_calibrator.h"
#include "nlohmann/json.hpp"

namespace nat2pro
{
    class MSGNatCalibrator : public satdump::products::ImageCalibrator
    {
    private:
        double calibration_slope[12];
        double calibration_offset[12];

    public:
        MSGNatCalibrator(satdump::products::ImageProduct *p, nlohmann::json c)
            : satdump::products::ImageCalibrator(p, c)
        {
            for (int i = 0; i < 12; i++)
            {
                calibration_slope[i] = c["vars"]["slope"][i];
                calibration_offset[i] = c["vars"]["offset"][i];
            }
        }

        void init()
        {
        }

        double compute(int channel, int pos_x, int pos_y, uint32_t px_val)
        {
            if (px_val == 0)
                return CALIBRATION_INVALID_VALUE;

            double physical_units = calibration_offset[channel] + px_val * calibration_slope[channel];

            return physical_units;
        }
    };
}
