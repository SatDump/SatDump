#pragma once

#include "products/image_products.h"
#include "nlohmann/json.hpp"
#include "common/calibration.h"
#include "common/projection/sat_proj/sat_proj.h"
#include "common/projection/reprojector.h"

namespace nc2pro
{
    class ABINcCalibrator : public satdump::ImageProducts::CalibratorBase
    {
    private:
        double *calibration_scale;
        double *calibration_offset;
        double *calibration_kappa;
        int *channel_lut;

    public:
        ABINcCalibrator(nlohmann::json calib, satdump::ImageProducts* products) : satdump::ImageProducts::CalibratorBase(calib, products)
        {
            size_t num_channels = products->images.size();
            calibration_scale = new double[num_channels];
            calibration_offset = new double[num_channels];
            calibration_kappa = new double[num_channels];
            channel_lut = new int[num_channels];

            for (int i = 0; i < num_channels; i++)
            {
                calibration_scale[i] = calib["vars"]["scale"][i];
                calibration_offset[i] = calib["vars"]["offset"][i];
                calibration_kappa[i] = calib["vars"]["kappa"][i];
                channel_lut[i] = std::stoi(products->images[i].channel_name) - 1;
            }
        }

        ~ABINcCalibrator()
        {
            delete[] calibration_scale;
            delete[] calibration_offset;
            delete[] calibration_kappa;
            delete[] channel_lut;
        }

        void init()
        {
        }

        double compute(int channel, int pos_x, int pos_y, int px_val)
        {
            if (px_val == 0)
                return CALIBRATION_INVALID_VALUE;

            channel = channel_lut[channel];

            if (calibration_offset[channel] == 0 || calibration_scale[channel] == 0)
                return CALIBRATION_INVALID_VALUE;

            double rad = calibration_offset[channel] + px_val * calibration_scale[channel];

            if (calibration_kappa[channel] > 0)
                return rad * calibration_kappa[channel];
            else
                return rad;
        }
    };
}
