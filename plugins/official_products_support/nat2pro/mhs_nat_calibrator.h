#pragma once

#include "products/image_products.h"
#include "nlohmann/json.hpp"
#include "common/calibration.h"
#include "common/projection/sat_proj/sat_proj.h"

namespace nat2pro
{
    class MHSNatCalibrator : public satdump::ImageProducts::CalibratorBase
    {

    public:
        MHSNatCalibrator(nlohmann::json calib, satdump::ImageProducts *products) : satdump::ImageProducts::CalibratorBase(calib, products)
        {
        }

        void init()
        {
        }

        double compute(int channel, int pos_x, int pos_y, int px_val)
        {
            if (px_val == 0)
                return CALIBRATION_INVALID_VALUE;

            double physical_units = double(px_val << 4);

            physical_units /= 1000 * 1000 * 10;

            return physical_units;
        }
    };
}
