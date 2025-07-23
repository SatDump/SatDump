#pragma once

#include "products/image/image_calibrator.h"
#include "nlohmann/json.hpp"
#include "core/exception.h"

namespace noaa_metop
{
    class NoaaMHSCalibrator : public satdump::products::ImageCalibrator
    {
    private:
        nlohmann::json perLine_perChannel;
        nlohmann::json perChannel;

        double calc_rad(int channel, int pos_y, int px_val)
        {
            if (px_val == 0 || perLine_perChannel[pos_y][channel]["a0"].get<double>() == -999.99)
                return CALIBRATION_INVALID_VALUE;
            return perLine_perChannel[pos_y][channel]["a0"].get<double>() + perLine_perChannel[pos_y][channel]["a1"].get<double>() * px_val + perLine_perChannel[pos_y][channel]["a2"].get<double>() * px_val * px_val;
            // return out_rad;
        }

    public:
        NoaaMHSCalibrator(satdump::products::ImageProduct *p, nlohmann::json c)
            : satdump::products::ImageCalibrator(p, c)
        {
            if (!d_cfg.contains("vars") || !d_cfg["vars"].contains("perLine_perChannel"))
                throw satdump_exception("Calibration data missing!");
            perLine_perChannel = d_cfg["vars"]["perLine_perChannel"];
        }

        double compute(int channel, int /*pos_x*/, int pos_y, uint32_t px_val)
        {
            try
            {
                return calc_rad(channel, pos_y, px_val);
            }
            catch (std::exception &)
            {
                return 0;
            }
        }
    };
}