#pragma once

#include "products/image/image_calibrator.h"

namespace nat2pro
{
    class AVHRRNatCalibrator : public satdump::products::ImageCalibrator
    {

    public:
        AVHRRNatCalibrator(satdump::products::ImageProduct *p, nlohmann::json c) : satdump::products::ImageCalibrator(p, c)
        {
        }

        double compute(int channel, int pos_x, int pos_y, uint32_t px_val)
        {
            if (px_val == 0)
                return CALIBRATION_INVALID_VALUE;

            if (channel == 2 || channel == 3)
                return (double(px_val) - 32768) / 10000;
            else
                return (double(px_val) - 32768) / 100;
        }
    };
}
