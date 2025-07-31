#pragma once

#include "common/calibration.h"
#include "nlohmann/json.hpp"
#include "products/image/image_calibrator.h"
#include <cstdio>
#include <string>

namespace nc2pro
{
    class ABINcCalibrator : public satdump::products::ImageCalibrator
    {
    private:
        std::map<int, double> calibration_scale;
        std::map<int, double> calibration_offset;
        std::map<int, double> calibration_kappa;
        bool is_spectral = false;
        std::map<int, double> wavenm;

    public:
        ABINcCalibrator(satdump::products::ImageProduct *p, nlohmann::json c) : satdump::products::ImageCalibrator(p, c)
        {
            size_t num_channels = p->images.size();
            for (size_t i = 0; i < num_channels; i++)
            {
                calibration_scale[p->images[i].abs_index] = c["vars"]["scale"][p->images[i].abs_index];
                calibration_offset[p->images[i].abs_index] = c["vars"]["offset"][p->images[i].abs_index];
                calibration_kappa[p->images[i].abs_index] = c["vars"]["kappa"][p->images[i].abs_index];
                wavenm[p->images[i].abs_index] = p->get_channel_wavenumber(p->images[i].abs_index);
            }
            if (c["vars"].contains("spectral"))
                is_spectral = c["vars"]["spectral"];
        }

        void init() {}

        double compute(int channel, int /* pos_x */, int /* pos_y */, uint32_t px_val)
        {
            if (px_val == 0)
                return CALIBRATION_INVALID_VALUE;

            if (calibration_offset[channel] == 0 || calibration_scale[channel] == 0)
                return CALIBRATION_INVALID_VALUE;

            double rad = calibration_offset[channel] + px_val * calibration_scale[channel];

            if (calibration_kappa[channel] > 0)
                return rad * calibration_kappa[channel];
            else
                return spectral_radiance_to_radiance(rad, wavenm[channel]);
        }
    };
} // namespace nc2pro
