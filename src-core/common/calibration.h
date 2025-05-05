#pragma once

#include <math.h>
#include <map>
#include <ctime>

#define c1 1.1910427e-05
#define c2 1.4387752
#define e_num 2.7182818

double temperature_to_radiance(double t, double v);

double radiance_to_temperature(double L, double v);

double freq_to_wavenumber(double freq);

double wavenumber_to_freq(double wavenumber);

inline double spectral_radiance_to_radiance(double L, double wavenumber)
{
    double c_1 = 1.191042e8;
    double c_2 = 1.4387752e4;
    double lamba = (1e7 / wavenumber) / 1e3;
    double temp = c_2 / (lamba * log(c_1 / (pow(lamba, 5) * L + 1)));
    return temperature_to_radiance(temp, wavenumber);
}

//////////////////////////////////////////////////////////////////////
//// Experimental
//////////////////////////////////////////////////////////////////////

double calculate_sun_irradiance_interval(double low_wav, double high_wav);

double radiance_to_reflectance(double irradiance, double radiance, time_t ltime, float lat, float lon);
