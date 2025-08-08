#pragma once

#include "common/calibration.h"
#include "nlohmann/json.hpp"
#include "products/image/image_calibrator.h"

namespace satdump
{
    namespace xrit
    {
        class MSUGSXritCalibrator : public satdump::products::ImageCalibrator
        {
        private:
            double calibration_lut[10][1024];
            double wavenm[10];

        public:
            MSUGSXritCalibrator(satdump::products::ImageProduct *p, nlohmann::json c) : satdump::products::ImageCalibrator(p, c)
            {
                for (int i = 0; i < 10; i++)

                    for (int y = 0; y < 1024; y++)

                        calibration_lut[i][y] = c["vars"]["lut"][i][y];

                for (auto &h : p->images)
                    wavenm[h.abs_index] = p->get_channel_wavenumber(h.abs_index);
            }

            double compute(int channel, int pos_x, int pos_y, uint32_t px_val)
            {
                if (px_val == 0 || px_val > 1023)
                    return CALIBRATION_INVALID_VALUE;

                double physical_units = calibration_lut[channel][px_val];

                if (channel >= 3)
                    physical_units = temperature_to_radiance(physical_units, wavenm[channel]);

                return physical_units;
            }
        };
    } // namespace xrit
} // namespace satdump
