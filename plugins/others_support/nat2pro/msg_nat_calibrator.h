#pragma once

#include "products/image_products.h"
#include "nlohmann/json.hpp"
#include "common/calibration.h"

namespace nat2pro
{
    class MSGNatCalibrator : public satdump::ImageProducts::CalibratorBase
    {
    private:
        double calibration_slope[12];
        double calibration_offset[12];

    public:
        MSGNatCalibrator(nlohmann::json calib, satdump::ImageProducts *products) : satdump::ImageProducts::CalibratorBase(calib, products)
        {
            for (int i = 0; i < 12; i++)
            {
                calibration_slope[i] = calib["vars"]["slope"][i];
                calibration_offset[i] = calib["vars"]["offset"][i];
            }
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

            // printf("VAL %f\n", final_val);
            // final_val = (d_products->get_wavenumber(channel) / final_val) * 1e1; //* 1e-3;
            // printf("VAL2 %f\n", final_val);

            double physical_units = calibration_offset[channel] + px_val * calibration_slope[channel];

            // From https://user.eumetsat.int/resources/user-guides/msg-high-rate-seviri-level-1-5-data-guide
            double c_1 = 1.19104e-5;
            double c_2 = 1.43877;

            // double bt =

            return physical_units; // CALIBRATION_INVALID_VALUE;
        }
    };
}
