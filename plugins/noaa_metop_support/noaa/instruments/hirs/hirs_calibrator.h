#pragma once

#include "products/image_products.h"
#include "nlohmann/json.hpp"
#include "core/exception.h"

class NoaaHIRSCalibrator : public satdump::ImageProducts::CalibratorBase
{
private:
    nlohmann::json perLine_perChannel;
    nlohmann::json perChannel;

    double calc_rad(int channel, int pos_y, int px_val)
    {
        if (channel == 19)
            return px_val == 0 ? CALIBRATION_INVALID_VALUE : (perChannel["a0"].get<double>() + perChannel["a1"].get<double>() * px_val > 1 ? CALIBRATION_INVALID_VALUE : perChannel["a0"].get<double>() + perChannel["a1"].get<double>() * px_val);

        if (px_val == 0 || perLine_perChannel[channel][pos_y]["a0"].get<double>() == -999.99)
            return CALIBRATION_INVALID_VALUE;

        return perLine_perChannel[channel][pos_y]["a0"].get<double>() + perLine_perChannel[channel][pos_y]["a1"].get<double>() * px_val;
    }

public:
    NoaaHIRSCalibrator(nlohmann::json calib, satdump::ImageProducts *products) : satdump::ImageProducts::CalibratorBase(calib, products)
    {
    }

    void init()
    {
        if (!d_calib.contains("vars") || !d_calib["vars"].contains("perLine_perChannel") || !d_calib["vars"].contains("perChannel"))
            throw satdump_exception("Calibration data missing!");
        perLine_perChannel = d_calib["vars"]["perLine_perChannel"];
        perChannel = d_calib["vars"]["perChannel"];
    }

    double compute(int channel, int /*pos_x*/, int pos_y, int px_val)
    {
        try
        {
            return calc_rad(channel, pos_y, px_val);
        }
        catch (std::exception &)
        {
            return -999.99;
        }
    }
};