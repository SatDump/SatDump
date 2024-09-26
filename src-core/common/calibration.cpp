#include "calibration.h"
#include <vector>
#include <ctime>

double temperature_to_radiance(double t, double v) { return (c1 * v * v * v) / (pow(e_num, c2 * v / t) - 1); }

double radiance_to_temperature(double L, double v) { return (c2 * v) / (log(c1 * v * v * v / L + 1)); }

double freq_to_wavenumber(double freq) { return 1e7 / ((299792458.0 / freq) / 10e-10); }

double wavenumber_to_freq(double wavenumber) { return 299792458.0 / ((1e7 / wavenumber) * 10e-10); }

//////////////////////////////////////////////////////////////////////
//// Experimental
//////////////////////////////////////////////////////////////////////

namespace
{
    double trapz(std::vector<double> els)
    {
        double h = 1;
        double i = 0;
        double sum = 0;

        for (i; i < els.size(); ++i)
        {
            if (i == 0 || i == els.size() - 1)
                sum += els[i] / 2;
            else
                sum += els[i];
        }
        return sum * h;
    }

    double irrad(double low_wav, double high_wav, double t_bb)
    {
        // IRRAD calculate radiant flux of the blackbody over some wavelength interval
        double h = 6.63e-34;  // J-s, Planck's constant
        double k = 1.38e-23;  // J/K, Boltzmann's constant
        double c = 299790000; // m/s, speed of light
        double dwav = (high_wav - low_wav) / 50.0;
        std::vector<double> E_wav;
        double current_wav = low_wav;
        for (int i = 0; i < 50; i++)
        {
            double ewav = temperature_to_radiance(t_bb, freq_to_wavenumber(299792458.0 / current_wav)); // dwav * (2.0 * M_PI * h * pow(c, 2)) / (pow(current_wav, 5) * (exp(h * c / (current_wav * k * t_bb)) - 1.0));
            E_wav.push_back(ewav);
            current_wav = low_wav + i * dwav;
        }
        return trapz(E_wav) / (48.0);
    }
}

double calculate_sun_irradiance_interval(double low_wav, double high_wav)
{
    double E_rad_sun = irrad(low_wav, high_wav, 5772);
    double r_eq_sun = 696000000; // m, equatorial radius of sun
    double r_AU = 149597870700;  // m, exact def of 1 AU
    double E_irrad = E_rad_sun * (pow(r_eq_sun, 2) / pow(r_AU, 2));
    return E_irrad;
}

namespace
{
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

    double cos_sol_za(int yr, int month, int day, int hour, int minute, double lat, double lon)
    {
        double hourz = (double)hour + ((double)minute) / 60.0;
        double jd = jday(yr, month, day);
        double zenith;

        zenith = cos_sol_za(jd, hourz, lat, lon);

        return zenith;
    }
}

// cos(80deg)
#define cos80 0.173648178

double radiance_to_reflectance(double irradiance, double radiance, time_t ltime, float lat, float lon)
{
    // if (chnum != 1 && chnum != 2 && chnum != 3 && chnum != 12)
    //     return CALIBRATION_INVALID_VALUE;

    std::tm t_read = *gmtime(&ltime);

    int year = t_read.tm_year + 1900;
    int month = t_read.tm_mon + 1;
    int day = t_read.tm_mday;
    int hour = t_read.tm_hour;
    int minute = t_read.tm_min;

    int jd = jday(year, month, day);
    double esd = 1.0 - 0.0167 * cos(2.0 * M_PI * (jd - 3) / 365.0);
    double oneoveresdsquare = 1.0 / (esd * esd);
    // double torad[4] = {20.76 * oneoveresdsquare, 23.24 * oneoveresdsquare,
    //                    19.85 * oneoveresdsquare, 25.11 * oneoveresdsquare};

    double tr = irradiance * oneoveresdsquare; // (chnum < 4) ? torad[chnum - 1] : torad[3];
    double cos_sza = cos_sol_za(year, month, day, hour, minute, lat, lon);
    // Use cos(80°) as lower bound, to avoid division by zero
    if (cos_sza < cos80)
        return -999.99; // CALIBRATION_INVALID_VALUE; // 0.05;    // cos80;
    return /*100.0 **/ radiance / tr / cos_sza;
}