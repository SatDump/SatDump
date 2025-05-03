#pragma once

#include "products2/image/calibration_converter.h"

namespace satdump
{
    namespace calibration
    {
        namespace conv
        {
            class RefRadToSunCorRefRadConverter : public ConverterBase
            {
            public:
                double convert(const UnitConverter *c, double x, double y, double val)
                {
                    geodetic::geodetic_coords_t pos;
                    double timestamp = -1;
                    if (((UnitConverter *)c)->proj.inverse(x, y, pos, &timestamp))
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