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

        int jday(int yr, int month, int day)
        {
            bool leap;
            double j = 0.0;

            if (((yr % 4) == 0 && (yr % 100) != 0) || (yr % 400) == 0)
                leap = true;
            else
                leap = false;

            if (month == 1)
                j = 0.0;
            if (month == 2)
                j = 31.0;
            if (month == 3)
                j = 59.0;
            if (month == 4)
                j = 90.0;
            if (month == 5)
                j = 120.0;
            if (month == 6)
                j = 151.0;
            if (month == 7)
                j = 181.0;
            if (month == 8)
                j = 212.0;
            if (month == 9)
                j = 243.0;
            if (month == 10)
                j = 273.0;
            if (month == 11)
                j = 304.0;
            if (month == 12)
                j = 334.0;
            if (month > 2 && leap)
                j = j + 1.0;

            j = j + day;
            return j;
        }

        // From MSG_data_RadiometricProc.cpp
        double cos_sol_za(int yr, int month, int day, int hour, int minute,
                          double lat, double lon)
        {
            double hourz = (double)hour + ((double)minute) / 60.0;
            double jd = jday(yr, month, day);
            double zenith;

            zenith = cos_sol_za(jd, hourz, lat, lon);

            return zenith;
        }

        double cos_sol_za(int jday, double hour, double dlat, double dlon)
        {
            // http://en.wikipedia.org/wiki/Solar_zenith_angle
            double coz;

            const double sinob = 0.3978;
            // Days per year
            const double dpy = 365.242;
            // Degrees per hour, speed of earth rotation
            const double dph = 15.0;
            // From degrees to radians
            const double rpd = M_PI / 180.0;
            // From radians to degrees
            const double dpr = 1.0 / rpd;
            // Angle in earth orbit in radians, 0 = the beginning of the year
            double dang = 2.0 * M_PI * (double)(jday - 1) / dpy;
            double homp = 12.0 + 0.123570 * sin(dang) - 0.004289 * cos(dang) +
                          0.153809 * sin(2.0 * dang) + 0.060783 * cos(2.0 * dang);
            // Hour angle in the local solar time (degrees)
            double hang = dph * (hour - homp) + dlon;
            double ang = 279.9348 * rpd + dang;
            double sigma = (ang * dpr + 0.4087 * sin(ang) + 1.8724 * cos(ang) -
                            0.0182 * sin(2.0 * ang) + 0.0083 * cos(2.0 * ang)) *
                           rpd;
            // Sin of sun declination
            double sindlt = sinob * sin(sigma);
            // Cos of sun declination
            double cosdlt = sqrt(1.0 - sindlt * sindlt);

            coz = sindlt * sin(rpd * dlat) + cosdlt * cos(rpd * dlat) * cos(rpd * hang);

            return coz;
        }

        // cos(80deg)
#define cos80 0.173648178

        float radiance_to_reflectance(int chnum, float radiance,
                                      int year, int month, int day,
                                      int hour, int minute,
                                      float lat, float lon)
        {
            if (chnum != 1 || chnum != 2 || chnum != 3 || chnum != 12)
                return CALIBRATION_INVALID_VALUE;

            int jd = jday(year, month, day);
            double esd = 1.0 - 0.0167 * cos(2.0 * M_PI * (jd - 3) / 365.0);
            double oneoveresdsquare = 1.0 / (esd * esd);
            double torad[4] = {20.76 * oneoveresdsquare, 23.24 * oneoveresdsquare,
                               19.85 * oneoveresdsquare, 25.11 * oneoveresdsquare};

            double tr = (chnum < 4) ? torad[chnum - 1] : torad[3];
            double cos_sza = cos_sol_za(year, month, day, hour, minute, lat, lon);
            // Use cos(80°) as lower bound, to avoid division by zero
            if (cos_sza < cos80)
                return cos80;
            return 100.0 * radiance / tr / cos_sza;
        }

        geodetic::geodetic_coords_t coords;
        std::shared_ptr<satdump::SatelliteProjection> projs[12];

    public:
        MSGNatCalibrator(nlohmann::json calib, satdump::ImageProducts *products) : satdump::ImageProducts::CalibratorBase(calib, products)
        {
            for (int i = 0; i < 12; i++)
            {
                calibration_slope[i] = calib["vars"]["slope"][i];
                calibration_offset[i] = calib["vars"]["offset"][i];

                nlohmann::json proj_cfg = products->get_proj_cfg();
                satdump::reprojection::rescaleProjectionScalarsIfNeeded(proj_cfg, products->images[i].image.width(), products->images[i].image.height());
                projs[i] = satdump::get_sat_proj(proj_cfg, products->get_tle(), {}, true);
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

            if (channel == 0 || channel == 1 || channel == 2 || channel == 11)
            {
                if (!projs[channel]->get_position(pos_x, pos_y, coords))
                {
                    coords.toDegs();
                    logger->trace("%f %f", coords.lon, coords.lat);
                    return radiance_to_reflectance(channel + 1, physical_units, 2024, 7, 6, 11, 45, coords.lat, coords.lon);
                }
                else
                {
                    return CALIBRATION_INVALID_VALUE;
                }
            }
            else
                return physical_units;
        }
    };
}
