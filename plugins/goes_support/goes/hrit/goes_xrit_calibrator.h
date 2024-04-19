#pragma once

#include "products/image_products.h"
#include "nlohmann/json.hpp"
#include "common/calibration.h"

namespace goes
{
    namespace hrit
    {
        class GOESxRITCalibrator : public satdump::ImageProducts::CalibratorBase
        {
        private:
            nlohmann::json calib_cfg;
            std::vector<double> wavenumbers;
            std::map<int, std::string> channel_map;
            std::map<int, satdump::ImageProducts::calib_type_t> calib_type_map;
            std::vector<int> new_max_val;

        public:
            GOESxRITCalibrator(nlohmann::json calib, satdump::ImageProducts *products) : satdump::ImageProducts::CalibratorBase(calib, products)
            {
                calib_cfg = calib;
                wavenumbers = calib["wavenumbers"].get<std::vector<double>>();
                for (int i = 0; i < d_products->images.size(); i++)
                {
                    channel_map.emplace(i, d_products->images[i].channel_name);
                    calib_type_map.emplace(i, d_products->get_calibration_type(i));

                    if (!calib_cfg["bits_for_calib"].is_null())
                        new_max_val.push_back(pow(2, calib_cfg["bits_for_calib"].get<int>()) - 1);
                    else
                        new_max_val.push_back(255);
                }
            }

            void init()
            {
            }

            double compute(int channel, int /*pos_x*/, int /*pos_y*/, int px_val)
            {
                if (new_max_val[channel] != 255)
                    px_val = double(px_val / 255.0) * new_max_val[channel];

                try
                {
                    if (calib_type_map[channel] == d_products->CALIB_REFLECTANCE)
                    {
                        if (px_val == 255)
                            return CALIBRATION_INVALID_VALUE;
                        else
                            return calib_cfg[channel_map[channel]][px_val];
                    }
                    else if (calib_type_map[channel] == d_products->CALIB_RADIANCE)
                    {
                        if (px_val == 0)
                            return CALIBRATION_INVALID_VALUE;
                        else
                            return temperature_to_radiance(calib_cfg[channel_map[channel]][px_val], wavenumbers[channel]);
                    }
                    else
                        return CALIBRATION_INVALID_VALUE;
                }
                catch (std::exception &e)
                {
                    return CALIBRATION_INVALID_VALUE;
                }
            }
        };
    }
}