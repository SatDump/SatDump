#pragma once

/**
 * @file em_rad_to_bright_temp.h
 */

#include "products2/image/calibration_converter.h"

namespace satdump
{
    namespace calibration
    {
        namespace conv
        {
            /**
             * @brief Emissive Radiance to Brightness
             * Temperature converter
             *
             * Given a wavenumber, this will convert a
             * radiance value to its equivalent brightness
             * temperature in Kelvins.
             */
            class EmRadToBrightTempConverter : public ConverterBase
            {
            private:
                const bool celcius = false;

            public:
                EmRadToBrightTempConverter(bool celcius) : celcius(celcius) {}

                double convert(const UnitConverter *c, double x, double y, double val)
                {
                    if (c->wavenumber == -1)
                        return CALIBRATION_INVALID_VALUE;
                    return radiance_to_temperature(val, c->wavenumber) - (celcius ? 273.15 : 0.0);
                }

                bool convert_range(const UnitConverter *c, double &min, double &max)
                {
                    min = convert(c, 0, 0, min);
                    max = convert(c, 0, 0, max);
                    return true;
                }
            };
        } // namespace conv
    } // namespace calibration
} // namespace satdump