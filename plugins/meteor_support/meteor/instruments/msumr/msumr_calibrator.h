#pragma once

#include "products2/image/image_calibrator.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_utils.h"
#include "common/calibration.h"
#include "common/utils.h"

namespace meteor
{
    class MeteorMsuMrCalibrator : public satdump::products::ImageCalibrator
    {
    private:
        bool lrpt;
        // todo
        std::vector<double> wavenumbers;

        std::vector<std::vector<std::pair<uint16_t, uint16_t>>> views;
        std::vector<double> cold_temps;
        std::vector<double> hot_temps;

    public:
        MeteorMsuMrCalibrator(satdump::products::ImageProduct *p, nlohmann::json c) : satdump::products::ImageCalibrator(p, c)
        {
            lrpt = getValueOrDefault(d_cfg["vars"]["lrpt"], false);
            views.resize(d_pro->images.size());

            int max_lcnt = 0;
            for (size_t i = 0; i < d_pro->images.size(); i++)
            {
                wavenumbers.push_back(d_pro->get_channel_wavenumber(d_pro->images[i].abs_index));
                int lcnt = d_cfg["vars"]["views"][i][0].size();
                max_lcnt = std::max(lcnt, max_lcnt);
                for (int j = 0; j < lcnt; j++)
                    views[i].push_back({d_cfg["vars"]["views"][i][0][j], d_cfg["vars"]["views"][i][1][j]});
            }

            for (int i = 0; i < max_lcnt; i++)
            {
                double coldt = 0;
                double hott = 0;
                int next_x_calib = i;
                while (next_x_calib < max_lcnt)
                {
                    if (!d_cfg["vars"]["temps"][next_x_calib].is_null())
                    {
                        coldt = (d_cfg["vars"]["temps"][next_x_calib]["analog_tlm"]["cold_temp1"].get<double>() +
                                 d_cfg["vars"]["temps"][next_x_calib]["analog_tlm"]["cold_temp2"].get<double>()) /
                                2.0;
                        hott = (d_cfg["vars"]["temps"][next_x_calib]["analog_tlm"]["hot_temp1"].get<double>() +
                                d_cfg["vars"]["temps"][next_x_calib]["analog_tlm"]["hot_temp2"].get<double>()) /
                               2.0;
                        break;
                    }
                    next_x_calib++;
                }

                if (coldt == 0 || hott == 0)
                {
                    int next_x_calib = i;
                    while (0 < next_x_calib)
                    {
                        if (!d_cfg["vars"]["temps"][next_x_calib].is_null())
                        {
                            coldt = (d_cfg["vars"]["temps"][next_x_calib]["analog_tlm"]["cold_temp1"].get<double>() +
                                     d_cfg["vars"]["temps"][next_x_calib]["analog_tlm"]["cold_temp2"].get<double>()) /
                                    2.0;
                            hott = (d_cfg["vars"]["temps"][next_x_calib]["analog_tlm"]["hot_temp1"].get<double>() +
                                    d_cfg["vars"]["temps"][next_x_calib]["analog_tlm"]["hot_temp2"].get<double>()) /
                                   2.0;
                            break;
                        }
                        next_x_calib--;
                    }
                }

                cold_temps.push_back(coldt);
                hot_temps.push_back(hott);
            }

            auto coldm = most_common(cold_temps.begin(), cold_temps.end(), 0);
            auto hotm = most_common(hot_temps.begin(), hot_temps.end(), 0);
            for (int i = 0; i < max_lcnt; i++)
            {
                if (abs(coldm - cold_temps[i]) > 5)
                    cold_temps[i] = coldm;
                if (abs(hotm - hot_temps[i]) > 5)
                    hot_temps[i] = hotm;
            }
        }

        double compute(int channel, int /*pos_x*/, int pos_y, uint32_t px_val)
        {
            if (wavenumbers[channel] == 0)
                return CALIBRATION_INVALID_VALUE;

            if (lrpt)
            {
                pos_y /= 8;                                  // Telemetry data covers 8 lines
                px_val = ((float)px_val / 255.0f) * 1023.0f; // Scale to 10-bit
            }

            double cold_view = views[channel][pos_y].first;
            double hot_view = views[channel][pos_y].second;
            if (cold_view == 0 || hot_view == 0 || px_val == 0)
                return CALIBRATION_INVALID_VALUE;

            double cold_ref = cold_temps[pos_y]; // 225;
            double hot_ref = hot_temps[pos_y];   // 312;
            double wavenumber = wavenumbers[channel];

            double cold_rad = temperature_to_radiance(cold_ref, wavenumber);
            double hot_rad = temperature_to_radiance(hot_ref, wavenumber);

            double gain_rad = (hot_rad - cold_rad) / (hot_view - cold_view);
            double rad = cold_rad + (px_val - cold_view) * gain_rad; // hot_rad + ((px_val - hot_view) / gain_rad);

            // printf("VIEW %f, %f %f %f %f (%f)=> %f\n", double(px_val), hot_view, cold_view, hot_rad, cold_rad, gain_rad, rad);

            return rad;
        }
    };
}