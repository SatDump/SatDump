#pragma once

/**
 * @file bright_temp_to_em_rad.h
 */

#include "products/image/calibration_converter.h"

namespace satdump
{
    namespace calibration
    {
        namespace conv
        {
            /**
             * @brief Brightness Temperate to Emissive
             * Radiance converter
             *
             * Given a wavenumber, this will convert a
             * brightness temperature in kelvins to its
             * equivalent radiance
             */
            class BrightTempToEmRadConverter : public ConverterBase
            {
            public:
                double convert(const UnitConverter *c, double x, double y, double val)
                {
                    if (val == CALIBRATION_INVALID_VALUE)
                        return val; // Special case

                    if (c->wavenumber == -1)
                        return CALIBRATION_INVALID_VALUE;
                    return temperature_to_radiance(val, c->wavenumber);
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