#pragma once

#ifdef BUILD_TRACKING

#include "common/geodetic/geodetic_coordinates.h"
#define xkmper 6.378135E3 /* Earth radius km */

/*
Those functions were taken from GPredict
*/
double arccos(double x, double y);
bool mirror_lon(geodetic::geodetic_coords_t sat, double rangelon, double &mlon, double mapbreak);
bool north_pole_is_covered(geodetic::geodetic_coords_t sat, float footprint);

#endif