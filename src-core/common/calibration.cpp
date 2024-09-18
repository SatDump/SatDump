#include "calibration.h"

double temperature_to_radiance(double t, double v) { return (c1 * v * v * v) / (pow(e_num, c2 * v / t) - 1); }

double radiance_to_temperature(double L, double v) { return (c2 * v) / (log(c1 * v * v * v / L + 1)); }

double freq_to_wavenumber(double freq) { return 1e7 / ((299792458.0 / freq) / 10e-10); }

double wavenumber_to_freq(double wavenumber) { return 299792458.0 / ((1e7 / wavenumber) * 10e-10); }
