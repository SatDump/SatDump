#include "predict.h"
#include <math.h>

/* Corrections for atmospheric refraction */
/* Reference:  Astronomical Algorithms by Jean Meeus, pp. 101-104    */
/* 			   http://en.wikipedia.org/wiki/Atmospheric_refraction */

#define A (1.02*M_PI/180.0)
#define B (10.3*M_PI/180.0)
#define C (5.11*M_PI/180.0)

double predict_refraction(double el)
{
	return A / tan( el + B / ( el + C ) );
}

double predict_refraction_ext(double el, double pressure, double temp)
{
	double x = 283*pressure / (101 * (273 + temp));
	return x * predict_refraction(el);
}

double predict_refraction_from_apparent(double apparent_el)
{
	return 1.0 / tan( apparent_el + 7.31*M_PI/180.0 / (apparent_el + 4.4*M_PI/180.0));
}

double predict_refraction_from_apparent_ext(double apparent_el, double pressure, double temp)
{
	double x = 283*pressure / (101 * (273 + temp));
	return x / tan( apparent_el + 7.31*M_PI/180.0 / (apparent_el + 4.4*M_PI/180.0));
}

double predict_refraction_rate(double el, double el_rate)
{
	double u0 = el + C;
	double u1 = sin(el + B / (el + C));
	return A * el_rate * (B / (u0*u0) - 1.0) / (u1*u1);
}

double predict_refraction_rate_ext(double el, double el_rate, double pressure, double temp)
{
	double x = 283*pressure / (101 * (273 + temp));
	return x * predict_refraction_rate(el, el_rate);
}

double predict_apparent_elevation(double el)
{
	double apparent = el + predict_refraction(el);
	if (apparent >= 0.0) return apparent;
	else return el;
}

double predict_apparent_elevation_ext(double el, double pressure, double temp)
{
	double apparent = el + predict_refraction_ext(el, pressure, temp);
	if (apparent >= 0.0) return apparent;
	else return el;
}

double predict_apparent_elevation_rate(double el, double el_rate)
{
	return el_rate * (1 + predict_refraction_rate(el, el_rate));
}

double predict_apparent_elevation_rate_ext(double el, double el_rate, double pressure, double temp)
{
	return el_rate * (1 + predict_refraction_rate_ext(el, el_rate, pressure, temp));
}

