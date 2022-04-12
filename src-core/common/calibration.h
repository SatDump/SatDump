#pragma once

#include <math.h>

#define c1 1.191042e-05
#define c2 1.4387752
#define e_num 2.7182818

double temperature_to_radiance(double t, double v);

double radiance_to_temperature(double L, double v);