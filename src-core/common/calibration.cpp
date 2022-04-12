#include "calibration.h"

double temperature_to_radiance(double t, double v) { return (c1 * pow(v, 3.0)) / (pow(e_num, c2 * v / t) - 1); }

double radiance_to_temperature(double L, double v) { return (c2 * v) / (log(c1 * pow(v, 3) / L + 1)); }