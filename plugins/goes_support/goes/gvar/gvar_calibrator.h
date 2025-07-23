#pragma once

#include "common/calibration.h"
#include "nlohmann/json.hpp"
#include "products/image/calibration_units.h"
#include "products/image/image_calibrator.h"

namespace goes
{
    namespace gvar
    {
        class GvarCalibrator : public satdump::products::ImageCalibrator
        {
        private:
            nlohmann::json calib_cfg;
            std::vector<double> wavenumbers;

            std::array<float, 1024> ch2_lut;
            std::array<float, 1024> ch3_lut;
            std::array<float, 1024> ch4_lut;
            std::array<float, 1024> ch5_lut;

        public:
            GvarCalibrator(satdump::products::ImageProduct *p, nlohmann::json c) : satdump::products::ImageCalibrator(p, c)
            {
                calib_cfg = d_cfg;
                for (size_t i = 0; i < d_pro->images.size(); i++)
                    wavenumbers.push_back(d_pro->images[i].wavenumber);
                ch2_lut = c["ch2_lut"];
                ch3_lut = c["ch3_lut"];
                ch4_lut = c["ch4_lut"];
                ch5_lut = c["ch5_lut"];
            }

            double compute(int channel, int /*pos_x*/, int /*pos_y*/, uint32_t px_val)
            {
                if (px_val > 1023)
                    return CALIBRATION_INVALID_VALUE;

                if (channel == 0)
                    return px_val / 1023.0;
                else if (channel == 1)
                    return ch2_lut[px_val];
                else if (channel == 2)
                    return ch3_lut[px_val];
                else if (channel == 3)
                    return ch4_lut[px_val];
                else if (channel == 4)
                    return ch5_lut[px_val];
                else
                    return CALIBRATION_INVALID_VALUE;
            }
        };
    } // namespace gvar
} // namespace goes