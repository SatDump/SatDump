#pragma once

#include "products/image_products.h"
#include "nlohmann/json.hpp"
#include "common/calibration.h"
#include "common/projection/sat_proj/sat_proj.h"

namespace nat2pro
{
    class AVHRRNatCalibrator : public satdump::ImageProducts::CalibratorBase
    {

    public:
        AVHRRNatCalibrator(nlohmann::json calib, satdump::ImageProducts *products) : satdump::ImageProducts::CalibratorBase(calib, products)
        {
        }

        void init()
        {
        }

        double compute(int channel, int pos_x, int pos_y, int px_val)
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
