#pragma once

#include "products/image_products.h"
#include "nlohmann/json.hpp"
#include "common/calibration.h"

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

        double compute(int channel, int /*pos_x*/, int /*pos_y*/, int px_val)
        {
            double final_val = px_val << 4;

            //  if (channel == 0)
            //      final_val = (final_val / d_products->get_wavenumber(channel)) * 1e-5 * 2;
            //  else if (channel == 1)
            //      final_val = (final_val / d_products->get_wavenumber(channel)) * 1e-5 * 4;

            printf("VAL %f\n", final_val);
            final_val = (d_products->get_wavenumber(channel) / final_val) * 1e1; //* 1e-3;
            printf("VAL2 %f\n", final_val);

            return final_val;
        }
    };
}
