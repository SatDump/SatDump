#pragma once

#include "products/image_products.h"
#include "nlohmann/json.hpp"
#include "common/calibration.h"

namespace metop
{
    namespace iasi
    {
        class MetOpIASIImagingCalibrator : public satdump::ImageProducts::CalibratorBase
        {
        private:
            nlohmann::json d_vars;
            double wavenum;

        public:
            MetOpIASIImagingCalibrator(nlohmann::json calib, satdump::ImageProducts *products) : satdump::ImageProducts::CalibratorBase(calib, products)
            {
                d_vars = calib["vars"];
                wavenum = products->get_wavenumber(0);
            }

            void init()
            {
            }

            double compute(int channel, int pos_x, int pos_y, int px_val)
            {
                int fpos_y = pos_y / 64;
                int fpos_x = pos_x / 64;
                int ifov_pos_y = pos_y % 64;
                int ifov_pos_x = pos_x % 64;

                if (px_val == 0)
                    return CALIBRATION_INVALID_VALUE;

                double bbt_temp = d_vars[fpos_y]["bbt"];
                // printf("%d %d %d %d - %f\n", fpos_y, fpos_x, ifov_pos_y, ifov_pos_x, bbt_temp);
                if (bbt_temp == 0)
                    return CALIBRATION_INVALID_VALUE;

                double cold_cnt = d_vars[fpos_y]["cold_counts"];
                // printf("%d %d %d %d - %f\n", fpos_y, fpos_x, ifov_pos_y, ifov_pos_x, cold_cnt);
                if (cold_cnt == 0)
                    return CALIBRATION_INVALID_VALUE;

                double warm_cnt = d_vars[fpos_y]["warm_counts"];
                // printf("%d %d %d %d - %f\n", fpos_y, fpos_x, ifov_pos_y, ifov_pos_x, warm_cnt);
                if (warm_cnt == 0)
                    return CALIBRATION_INVALID_VALUE;

                double space_radiance = temperature_to_radiance(2.73, wavenum);
                double warm_radiance = temperature_to_radiance(bbt_temp, wavenum);

                double gain_rad = (warm_cnt - cold_cnt) / (warm_radiance - space_radiance);
                double linear_radiance = warm_radiance + ((px_val - warm_cnt) / gain_rad);

                return linear_radiance;
            }
        };
    }
}
