#pragma once

#include "common/calibration.h"
#include "nlohmann/json.hpp"
#include "products/image/image_calibrator.h"
#include <cstdio>
#include <string>

namespace satdump
{
    namespace official
    {
        class GPMHdfCalibrator : public satdump::products::ImageCalibrator
        {
        private:
            std::map<int, double> wavenm;

        public:
            GPMHdfCalibrator(satdump::products::ImageProduct *p, nlohmann::json c) : satdump::products::ImageCalibrator(p, c)
            {
                size_t num_channels = p->images.size();
                for (size_t i = 0; i < num_channels; i++)
                    wavenm[p->images[i].abs_index] = p->get_channel_wavenumber(p->images[i].abs_index);
            }

            void init() {}

            double compute(int channel, int /* pos_x */, int /* pos_y */, uint32_t px_val)
            {
                if (px_val == 0)
                    return CALIBRATION_INVALID_VALUE;

                return temperature_to_radiance((double)px_val / 100.0, wavenm[channel]);
            }
        };
    } // namespace official
} // namespace satdump