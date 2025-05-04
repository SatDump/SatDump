#pragma once

#include "products2/image/calibration_converter.h"

namespace satdump
{
    namespace calibration
    {
        namespace conv
        {
            class BrightTempToEmRadConverter : public ConverterBase
            {
            public:
                double convert(const UnitConverter *c, double x, double y, double val)
                {
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