#pragma once

/**
 * @file sun_angle.h
 */

#include "products2/image/calibration_converter.h"

namespace satdump
{
    namespace calibration
    {
        namespace conv
        {
            /**
             * @brief Sun angle converter.
             *
             * This does not actually "convert" from anything,
             * but simply returns the calculated Solar Zenith Angle
             * for the requested image pixel.
             */
            class SunAngleConverter : public ConverterBase
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

                    return get_sun_angle(timestamp, pos.lat, pos.lon);
                }

                bool convert_range(const UnitConverter *c, double &min, double &max)
                {
                    min = 0;
                    max = 90;
                    return true;
                }
            };
        } // namespace conv
    } // namespace calibration
} // namespace satdump