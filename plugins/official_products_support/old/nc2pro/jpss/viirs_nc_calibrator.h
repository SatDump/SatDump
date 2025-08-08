#pragma once

#include "common/calibration.h"
#include "nlohmann/json.hpp"
#include "products/image/calibration_units.h"
#include "products/image/image_calibrator.h"

namespace nc2pro
{
    class VIIRSNcCalibrator : public satdump::products::ImageCalibrator
    {
    private:
        std::map<int, double> calibration_scale;
        std::map<int, double> calibration_offset;
        std::map<int, double> wavenumbers;

    public:
        VIIRSNcCalibrator(satdump::products::ImageProduct *p, nlohmann::json c) : satdump::products::ImageCalibrator(p, c)
        {
            calibration_scale = c["vars"]["scale"];
            calibration_offset = c["vars"]["offset"];

            for (auto &p : p->images)
                wavenumbers.emplace(p.abs_index, p.wavenumber);
        }

        void init() {}

        double compute(int channel, int pos_x, int pos_y, uint32_t px_val)
        {
            if (px_val == 0)
                return CALIBRATION_INVALID_VALUE;

            if (calibration_scale.count(channel) == 0)
                return CALIBRATION_INVALID_VALUE;

            double physical_units = calibration_offset[channel] + px_val * calibration_scale[channel];

            return spectral_radiance_to_radiance(physical_units, wavenumbers[channel]);
        }
    };
} // namespace nc2pro
