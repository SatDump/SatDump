#pragma once

#include "products/image_products.h"
#include "nlohmann/json.hpp"

class NoaaAVHRR3Calibrator : public satdump::ImageProducts::CalibratorBase
{
private:
    nlohmann::json perLine_perChannel;
    nlohmann::json perChannel;
    double crossover[3];

    double calc_rad(int channel, int pos_y, int px_val)
    {
        double nlin = perLine_perChannel[pos_y][channel]["Ns"].get<double>() +
                      (perLine_perChannel[pos_y][channel]["Nbb"].get<double>() - perLine_perChannel[pos_y][channel]["Ns"].get<double>()) *
                          (perLine_perChannel[pos_y][channel]["Spc"].get<double>() - px_val) /
                          (perLine_perChannel[pos_y][channel]["Spc"].get<double>() - perLine_perChannel[pos_y][channel]["Blb"].get<double>());
        double out_rad = nlin + perChannel[channel]["b"][0].get<double>() + perChannel[channel]["b"][1].get<double>() * nlin +
                         perChannel[channel]["b"][2].get<double>() * nlin * nlin;
        return out_rad;
    }

public:
    NoaaAVHRR3Calibrator(nlohmann::json calib, satdump::ImageProducts *products) : satdump::ImageProducts::CalibratorBase(calib, products)
    {
    }

    void init()
    {
        perLine_perChannel = d_calib["vars"]["perLine_perChannel"];
        perChannel = d_calib["vars"]["perChannel"];
        for (int i = 0; i < 3; i++)
            crossover[i] = (perChannel[i]["int_hi"].get<double>() - perChannel[i]["int_lo"].get<double>()) / (perChannel[i]["slope_lo"].get<double>() - perChannel[i]["slope_hi"].get<double>());
    }

    double compute(int channel, int /*pos_x*/, int pos_y, int px_val)
    {
        if (channel < 3)
        {
            if (px_val <= crossover[channel])
                return (perChannel[channel]["slope_lo"].get<double>() * px_val + perChannel[channel]["int_lo"].get<double>()) / 100.0;
            else
                return (perChannel[channel]["slope_hi"].get<double>() * px_val + perChannel[channel]["int_hi"].get<double>()) / 100.0;
        }
        else
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
    }
};