#pragma once

#include "common/calibration.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_utils.h"
#include "products/image/image_calibrator.h"
#include "utils/stats.h"

namespace aws
{
    namespace mwr
    {
        class AWSMWRCalibrator : public satdump::products::ImageCalibrator
        {
        private:
            std::vector<double> wavenumbers;

            std::vector<std::array<float, 4>> cal_load_temps;
            std::vector<std::array<std::array<uint16_t, 15>, 19>> cal_load_views;
            std::vector<std::array<std::array<uint16_t, 25>, 19>> cal_cold_views;

        public:
            AWSMWRCalibrator(satdump::products::ImageProduct *p, nlohmann::json c) : satdump::products::ImageCalibrator(p, c)
            {
                cal_load_temps = c["temps"];
                cal_load_views = c["load_views"];
                cal_cold_views = c["cold_views"];

                for (size_t i = 0; i < d_pro->images.size(); i++)
                    wavenumbers.push_back(d_pro->get_channel_wavenumber(d_pro->images[i].abs_index));
            }

            double compute(int channel, int /*pos_x*/, int pos_y, uint32_t px_val)
            {
                if (wavenumbers[channel] == 0 || px_val == 0)
                    return CALIBRATION_INVALID_VALUE;

                // Average cold views
                double cold_view = 0;
                for (auto &v : cal_cold_views[pos_y][channel])
                    cold_view += v;
                cold_view /= 25;

                // Average hot view
                double hot_view = 0;
                for (auto &v : cal_load_views[pos_y][channel])
                    hot_view += v;
                hot_view /= 15;

                if (cold_view == 0 || hot_view == 0 || px_val == 0)
                    return CALIBRATION_INVALID_VALUE;

                // Get cold/hot temps
                double cold_ref = 2.73;
                // Broken we need to figure out
                double hot_ref = 283; //(cal_load_temps[pos_y][0] + cal_load_temps[pos_y][1] + cal_load_temps[pos_y][2] + cal_load_temps[pos_y][3]) / 4.0 + 273.15;
                double wavenumber = wavenumbers[channel];

                double cold_rad = temperature_to_radiance(cold_ref, wavenumber);
                double hot_rad = temperature_to_radiance(hot_ref, wavenumber);

                double gain_rad = (hot_rad - cold_rad) / (hot_view - cold_view);
                double rad = cold_rad + (px_val - cold_view) * gain_rad; // hot_rad + ((px_val - hot_view) / gain_rad);

                // printf("VIEW %f, %f %f %f %f (%f)=> %f\n", double(px_val), hot_view, cold_view, hot_rad, cold_rad, gain_rad, rad);

                return rad;
            }
        };
    } // namespace mwr
} // namespace aws