#pragma once

/**
 * @file kelvin_celcius.h
 */

#include "products/image/calibration_converter.h"

namespace satdump
{
    namespace calibration
    {
        namespace conv
        {
            /**
             * @brief Kelvin <=> Celsius converter
             *
             * Converts between Kelvin and Celsius
             */
            class KelvinCelsiusConverter : public ConverterBase
            {
            private:
                const bool ktc;

            public:
                /**
                 * @brief Constructor
                 *
                 * @param ktc if true, Kelvin => Celcius
                 */
                KelvinCelsiusConverter(bool ktc) : ktc(ktc) {}

                double convert(const UnitConverter *c, double x, double y, double val)
                {
                    if (val == CALIBRATION_INVALID_VALUE)
                        return val; // Special case

                    return val + (ktc ? -273.15 : 273.15);
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