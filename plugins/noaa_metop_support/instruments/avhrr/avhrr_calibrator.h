#pragma once

#include "products/image/image_calibrator.h"
#include "nlohmann/json.hpp"

namespace noaa_metop
{
    class NoaaAVHRR3Calibrator : public satdump::products::ImageCalibrator
    {
    private:
        nlohmann::json perLine_perChannel;
        nlohmann::json perChannel;
        double crossover[3];
        bool per_line;
        int factor;

        double calc_rad(int channel, int pos_y, int px_val)
        {
            if (per_line)
            {
                double nlin = perLine_perChannel[pos_y][channel]["Ns"].get<double>() +
                              (perLine_perChannel[pos_y][channel]["Nbb"].get<double>() - perLine_perChannel[pos_y][channel]["Ns"].get<double>()) *
                                  (perLine_perChannel[pos_y][channel]["Spc"].get<double>() - px_val) /
                                  (perLine_perChannel[pos_y][channel]["Spc"].get<double>() - perLine_perChannel[pos_y][channel]["Blb"].get<double>());
                double out_rad = nlin + perChannel[channel]["b"][0].get<double>() + perChannel[channel]["b"][1].get<double>() * nlin +
                                 perChannel[channel]["b"][2].get<double>() * nlin * nlin;
                return out_rad;
            }
            else
            {
                double nlin = perChannel[channel]["Ns"].get<double>() +
                              (perChannel[channel]["Nbb"].get<double>() - perChannel[channel]["Ns"].get<double>()) *
                                  (perChannel[channel]["Spc"].get<double>() - px_val) /
                                  (perChannel[channel]["Spc"].get<double>() - perChannel[channel]["Blb"].get<double>());
                double out_rad = nlin + perChannel[channel]["b"][0].get<double>() + perChannel[channel]["b"][1].get<double>() * nlin +
                                 perChannel[channel]["b"][2].get<double>() * nlin * nlin;
                return out_rad;
            }
        }

    public:
        NoaaAVHRR3Calibrator(satdump::products::ImageProduct *p, nlohmann::json c)
            : satdump::products::ImageCalibrator(p, c)
        {
            per_line = d_cfg["vars"].contains("perLine_perChannel");
            if (per_line)
                perLine_perChannel = d_cfg["vars"]["perLine_perChannel"];
            perChannel = d_cfg["vars"]["perChannel"];
            for (int i = 0; i < 3; i++)
                crossover[i] = (perChannel[i]["int_hi"].get<double>() - perChannel[i]["int_lo"].get<double>()) / (perChannel[i]["slope_lo"].get<double>() - perChannel[i]["slope_hi"].get<double>());

            factor = pow(2, 10 - d_pro->images[0].bit_depth); // All channels got the same bit hdepth
        }

        double radiance_factors[3] = {1.0345143074006786, 1.2401744729666442, 1.3026239067055392}; // MetOp-C

        double compute(int channel, int /*pos_x*/, int pos_y, uint32_t px_val)
        {
            if (channel > 5 || px_val == 0)
                return CALIBRATION_INVALID_VALUE;
            if (channel < 3)
            {
                double reflectance = 0;
                int px = px_val * factor;
                if (!perChannel[channel].contains("slope_lo"))
                    return CALIBRATION_INVALID_VALUE;
                if (px <= crossover[channel])
                    reflectance = (perChannel[channel]["slope_lo"].get<double>() * px + perChannel[channel]["int_lo"].get<double>()) / 100.0;
                else
                    reflectance = (perChannel[channel]["slope_hi"].get<double>() * px + perChannel[channel]["int_hi"].get<double>()) / 100.0;

                if (!perChannel[channel].contains("F"))
                    return CALIBRATION_INVALID_VALUE;
                else
                    return (perChannel[channel]["F"].get<double>() / M_PI) * reflectance * radiance_factors[channel];
            }
            else
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
        }
    };
}