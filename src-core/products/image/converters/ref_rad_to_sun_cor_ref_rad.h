#pragma once

/**
 * @file ref_rad_to_sun_cor_ref_rad.h
 */

#include "products/image/calibration_converter.h"
#include <exception>

namespace satdump
{
    namespace calibration
    {
        namespace conv
        {
            /**
             * @brief Reflective Radiance to Sun Corrected
             * Reflective Radiance converter
             *
             * This corrects reflective radiance to negate the
             * effects of varying solar irradiance/elevation.
             *
             * It is also used for albedo correction, as the
             * math is identical.
             */
            class RefRadToSunCorRefRadConverter : public ConverterBase
            {
            public:
                double convert(const UnitConverter *c, double x, double y, double val)
                {
                    if (val == CALIBRATION_INVALID_VALUE)
                        return val; // Special case

                    geodetic::geodetic_coords_t pos;
                    double timestamp = -1;
                    if (((UnitConverter *)c)->proj.inverse(x, y, pos, &timestamp, false))
                        return CALIBRATION_INVALID_VALUE;

                    if (timestamp == -1)
                        return CALIBRATION_INVALID_VALUE;

                    return compensate_radiance_for_sun(val, timestamp, pos.lat, pos.lon);
                }

                bool convert_range(const UnitConverter *, double &, double &) { return true; }
            };
        } // namespace conv
    } // namespace calibration
} // namespace satdump