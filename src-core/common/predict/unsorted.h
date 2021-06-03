#ifndef UNSORTED_H_
#define UNSORTED_H_

#include "predict.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
//#include <sys/time.h>
#include <time.h>
//#include <unistd.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

/**
 * Set three-element vector to specified components.
 *
 * \param v Output vector
 * \param x x-component
 * \param y y-component
 * \param z z-component
 **/
void vec3_set(double v[3], double x, double y, double z);

/**
 * Get length of vector.
 *
 * \param v Input vector
 * \return Euclidean length
 **/
double vec3_length(const double v[3]);

/**
 * Multiply vector by scalar.
 *
 * \param v Input vector. Overwritten by result
 * \param a Scalar to be multiplied into vector
 * \param r Resulting vector
 **/
void vec3_mul_scalar(const double v[3], double a, double r[3]);

/**
 * Subtract a vector 2 from vector 1.
 *
 * \param v1 Vector 1
 * \param v2 Vector 2
 * \param r Resulting vector
 **/
void vec3_sub(const double v1[3], const double v2[3], double *r);

/**
 * Dot product between two vectors.
 *
 * \param v Vector
 * \param u Another vector
 * \return Dot product
 **/
double vec3_dot(const double v[3], const double u[3]);

/**
 * Geodetic position structure used by SGP4/SDP4 code.
 **/
typedef struct	{
		   double lat, lon, alt, theta;
		}  geodetic_t;

/**
 * General three-dimensional vector structure used by SGP4/SDP4 code.
 **/
typedef struct	{
		   double x, y, z, w;
		}  vector_t;

/**
 * This function returns a substring based on the starting
 * and ending positions provided. Trims whitespaces.
 *
 * \param input_string Full input string
 * \param buffer_length Length of output_buffer
 * \param output_buffer Returned substring
 * \param start Start position
 * \param end End position
 * \return Pointer to output_buffer
 * \copyright GPLv2+
 **/
char *SubString(const char *input_string, int buffer_length, char *output_buffer, int start, int end);

/**
 * Returns square of a double.
 *
 * \copyright GPLv2+
 **/
double Sqr(double arg);

/**
 * Returns mod 2PI of argument.
 *
 * \copyright GPLv2+
 **/
double FMod2p(double x);

/* predict's old date/time management functions. */

/**
 * Converts the satellite's position and velocity vectors from normalized values to km and km/sec.
 *
 * \copyright GPLv2+
 **/
void Convert_Sat_State(double pos[3], double vel[3]);

/**
 * The function Julian_Date_of_Year calculates the Julian Date of Day 0.0 of {year}. This function is used to calculate the Julian Date of any date by using Julian_Date_of_Year, DOY, and Fraction_of_Day. Astronomical Formulae for Calculators, Jean Meeus, pages 23-25. Calculate Julian Date of 0.0 Jan year.
 *
 * \copyright GPLv2+
 **/
double Julian_Date_of_Year(double year);

/**
 * The function Julian_Date_of_Epoch returns the Julian Date of an epoch specified in the format used in the NORAD two-line element sets. It has been modified to support dates beyond the year 1999 assuming that two-digit years in the range 00-56 correspond to 2000-2056. Until the two-line element set format is changed, it is only valid for dates through 2056 December 31. Modification to support Y2K. Valid 1957 through 2056.
 *
 * \copyright GPLv2+
 **/
double Julian_Date_of_Epoch(double epoch);

/**
 * Reference:  The 1992 Astronomical Almanac, page B6.
 *
 * \copyright GPLv2+
 **/
double ThetaG_JD(double jd);

/**
 * Calculates the day number from m/d/y. Needed for orbit_decay.
 *
 * \copyright GPLv2+
 **/
long DayNum(int month, int day, int year);

/**
 * Procedure Calculate_LatLonAlt will calculate the geodetic position of an object given its ECI position pos and time. It is intended to be used to determine the ground track of a satellite.  The calculations  assume the earth to be an oblate spheroid as defined in WGS '72. Reference:  The 1992 Astronomical Almanac, page K12.
 *
 * \copyright GPLv2+
 **/
void Calculate_LatLonAlt(double time, const double pos[3], geodetic_t *geodetic);

/**
 * The procedures Calculate_Obs and Calculate_RADec calculate
 * the *topocentric* coordinates of the object with ECI position,
 * {pos}, and velocity, {vel}, from location {geodetic} at {time}.
 * The {obs_set} returned for Calculate_Obs consists of azimuth,
 * elevation, range, and range rate (in that order) with units of
 * radians, radians, kilometers, and kilometers/second, respectively.
 * The WGS '72 geoid is used and the effect of atmospheric refraction
 * (under standard temperature and pressure) is incorporated into the
 * elevation calculation; the effect of atmospheric refraction on
 * range and range rate has not yet been quantified.
 * The {obs_set} for Calculate_RADec consists of right ascension and
 * declination (in that order) in radians.  Again, calculations are
 * based on *topocentric* position using the WGS '72 geoid and
 * incorporating atmospheric refraction.
 *
 * \copyright GPLv2+
 **/
void Calculate_Obs(double time,  const double pos[3], const double vel[3], geodetic_t *geodetic, vector_t *obs_set);

/**
 * Reference:  Methods of Orbit Determination by Pedro Ramon Escobal, pp. 401-402
 *
 * \copyright GPLv2+
 **/
void Calculate_RADec(double time, const double pos[3], const double vel[3], geodetic_t *geodetic, vector_t *obs_set);

/**
 *
 * \copyright GPLv2+
 **/
void Calculate_User_PosVel(double time, geodetic_t *geodetic, double obs_pos[3], double obs_vel[3]);

/**
 * Modified version of acos, where arguments above 1 or below -1 yield acos(-1 or +1).
 * Used for guarding against floating point inaccuracies.
 *
 * \param arg Argument
 * \return Arc cosine of the argument
 **/
double acos_(double arg);

/**
 * Modified version of asin, where arguments above 1 or below -1 yield acos(-1 or +1).
 * Used for guarding against floating point inaccuracies.
 *
 * \param arg Argument
 * \return Arc sine of the argument
 **/
double asin_(double arg);


#endif
