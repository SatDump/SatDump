#pragma once

#include <ctime>
#include <map>
#include <math.h>

double temperature_to_radiance(double t, double v);

double radiance_to_temperature(double L, double v);

double freq_to_wavenumber(double freq);

double wavenumber_to_freq(double wavenumber);

double spectral_radiance_to_radiance(double L, double wavenumber);

//////////////////////////////////////////////////////////////////////
//// Experimental
//////////////////////////////////////////////////////////////////////

double calculate_sun_irradiance_interval(double low_wav, double high_wav);

double radiance_to_reflectance(double irradiance, double radiance, time_t ltime, float lat, float lon);

double compensate_radiance_for_sun(double radiance, time_t ltime, float lat, float lon);

double get_sun_angle(time_t ltime, float lat, float lon);