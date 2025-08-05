#pragma once

#include "products/image/image_calibrator.h"

namespace nat2pro
{
    class AMSUNatCalibrator : public satdump::products::ImageCalibrator
    {

    public:
        AMSUNatCalibrator(satdump::products::ImageProduct *p, nlohmann::json c) : satdump::products::ImageCalibrator(p, c)
        {
        }

        void init()
        {
        }

        double compute(int channel, int pos_x, int pos_y, uint32_t px_val)
        {
            if (px_val == 0)
                return CALIBRATION_INVALID_VALUE;

            double physical_units = double(px_val << 2);

            physical_units /= 1000 * 1000 * 10;

            return physical_units;
        }
    };
}
