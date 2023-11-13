#pragma once

#include "products/image_products.h"
#include "nlohmann/json.hpp"

class NoaaMHSCalibrator : public satdump::ImageProducts::CalibratorBase
{
private:
    nlohmann::json perLine_perChannel;
    nlohmann::json perChannel;
    double crossover[2];

    double calc_rad(int channel, int pos_y, int px_val)
    {
        return perLine_perChannel[pos_y][channel]["a0"].get<double>() + perLine_perChannel[pos_y][channel]["a1"].get<double>() * px_val + perLine_perChannel[pos_y][channel]["a2"].get<double>() * px_val * px_val;
        // return out_rad;
    }

public:
    NoaaMHSCalibrator(nlohmann::json calib, satdump::ImageProducts *products) : satdump::ImageProducts::CalibratorBase(calib, products)
    {
    }

    void init()
    {
        perLine_perChannel = d_calib["vars"]["perLine_perChannel"];
    }

    double compute(int channel, int /*pos_x*/, int pos_y, int px_val)
    {
        try
        {
            return calc_rad(channel, pos_y, px_val);
        }
        catch (std::exception &e)
        {
            return 0;
        }
    }
};