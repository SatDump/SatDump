#pragma once

#include "products/image_products.h"
#include "nlohmann/json.hpp"
#include "common/calibration.h"
#include "common/projection/sat_proj/sat_proj.h"
#include "common/projection/reprojector.h"

namespace nat2pro
{
    class MSGNatCalibrator : public satdump::ImageProducts::CalibratorBase
    {
    private:
        double calibration_slope[12];
        double calibration_offset[12];

        //    geodetic::geodetic_coords_t coords;
        //    std::shared_ptr<satdump::SatelliteProjection> projs[12];

        time_t acqu_time = 0;

        //   double irradiance_values[4];

    public:
        MSGNatCalibrator(nlohmann::json calib, satdump::ImageProducts *products) : satdump::ImageProducts::CalibratorBase(calib, products)
        {
            for (int i = 0; i < 12; i++)
            {
                calibration_slope[i] = calib["vars"]["slope"][i];
                calibration_offset[i] = calib["vars"]["offset"][i];

                //     nlohmann::json proj_cfg = products->get_proj_cfg();
                //     satdump::reprojection::rescaleProjectionScalarsIfNeeded(proj_cfg, products->images[i].image.width(), products->images[i].image.height());
                //     projs[i] = satdump::get_sat_proj(proj_cfg, products->get_tle(), {}, true);

                if (products->has_product_timestamp())
                    acqu_time = products->get_product_timestamp();

                //     bool official = true;

                //   irradiance_values[0] = official ? 20.76 : calculate_sun_irradiance_interval(0.56e-6, 0.71e-6);
                //   irradiance_values[1] = official ? 23.24 : calculate_sun_irradiance_interval(0.74e-6, 0.88e-6);
                //   irradiance_values[2] = official ? 19.85 : calculate_sun_irradiance_interval(1.50e-6, 1.78e-6);
                //   irradiance_values[3] = official ? 25.11 : calculate_sun_irradiance_interval(0.6e-6, 0.9e-6);
            }
        }

        void init()
        {
        }

        double compute(int channel, int pos_x, int pos_y, int px_val)
        {
            if (px_val == 0)
                return CALIBRATION_INVALID_VALUE;

            double physical_units = calibration_offset[channel] + px_val * calibration_slope[channel];

            /* if (channel == 0 || channel == 1 || channel == 2 || channel == 11)
             {
                 if (acqu_time == 0)
                     return CALIBRATION_INVALID_VALUE;

                 if (!projs[channel]->get_position(pos_x, pos_y, coords))
                 {
                     coords.toDegs();
                     float val = radiance_to_reflectance(channel >= 3 ? irradiance_values[3] : irradiance_values[channel],
                                                         physical_units, acqu_time, coords.lat, coords.lon);
                     // logger->trace("%f %f => %f => %f", coords.lon, coords.lat, physical_units, val);
                     return val;
                 }
                 else
                 {
                     return CALIBRATION_INVALID_VALUE;
                 }
             }
             else */
            return physical_units;
        }
    };
}
