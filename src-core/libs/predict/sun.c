#include "sun.h"
#include "unsorted.h"
#include "defs.h"

/**
 * The function Delta_ET has been added to allow calculations on the position of the sun.  It provides the difference between UT (approximately the same as UTC) and ET (now referred to as TDT). This function is based on a least squares fit of data from 1950 to 1991 and will need to be updated periodically. Values determined using data from 1950-1991 in the 1990 Astronomical Almanac.  See DELTA_ET.WQ1 for details.
 *
 * \copyright GPLv2+
 **/
double Delta_ET(double year)
{
	double delta_et;

	delta_et=26.465+0.747622*(year-1950)+1.886913*sin(2*M_PI*(year-1975)/33);

	return delta_et;
}

/**
 * Returns angle in radians from argument in degrees.
 *
 * \copyright GPLv2+
 **/
double Radians(double arg)
{
	/* Returns angle in radians from argument in degrees */
	return (arg*M_PI/180.0);
}

/**
 * Returns angle in degrees from argument in radians.
 *
 * \copyright GPLv2+
 **/
double Degrees(double arg)
{
	/* Returns angle in degrees from argument in radians */
	return (arg*180.0/M_PI);
}

void sun_predict(double time, double position[3])
{
	double jul_utc = time + JULIAN_TIME_DIFF;
	double mjd = jul_utc - 2415020.0;
	double year = 1900 + mjd / 365.25;
	double T = (mjd + Delta_ET(year) / SECONDS_PER_DAY) / 36525.0;
	double M = Radians(fmod(358.47583+fmod(35999.04975*T,360.0)-(0.000150+0.0000033*T)*Sqr(T),360.0));
	double L = Radians(fmod(279.69668+fmod(36000.76892*T,360.0)+0.0003025*Sqr(T),360.0));
	double e = 0.01675104-(0.0000418+0.000000126*T)*T;
	double C = Radians((1.919460-(0.004789+0.000014*T)*T)*sin(M)+(0.020094-0.000100*T)*sin(2*M)+0.000293*sin(3*M));
	double O = Radians(fmod(259.18-1934.142*T,360.0));
	double Lsa = fmod(L+C-Radians(0.00569-0.00479*sin(O)), 2*M_PI);
	double nu = fmod(M+C, 2*M_PI);
	double R = 1.0000002*(1.0-Sqr(e))/(1.0+e*cos(nu));
	double eps = Radians(23.452294-(0.0130125+(0.00000164-0.000000503*T)*T)*T+0.00256*cos(O));
	R = ASTRONOMICAL_UNIT_KM*R;

	position[0] = R*cos(Lsa);
	position[1] = R*sin(Lsa)*cos(eps);
	position[2] = R*sin(Lsa)*sin(eps);
}

void predict_observe_sun(const predict_observer_t *observer, double time, struct predict_observation *obs)
{

	// Find sun position
	double solar_vector[3];
	sun_predict(time, solar_vector);

	/* Zero vector for initializations */
	double zero_vector[3] = {0,0,0};

	/* Solar observed azimuth and elevation vector  */
	vector_t solar_set;

	geodetic_t geodetic;
	geodetic.lat = observer->latitude;
	geodetic.lon = observer->longitude;
	geodetic.alt = observer->altitude / 1000.0;
	geodetic.theta = 0.0;

	double jul_utc = time + JULIAN_TIME_DIFF;
	Calculate_Obs(jul_utc, solar_vector, zero_vector, &geodetic, &solar_set);

	double sun_azi = solar_set.x;
	double sun_ele = solar_set.y;

	double sun_range = 1.0+((solar_set.z-ASTRONOMICAL_UNIT_KM)/ASTRONOMICAL_UNIT_KM);
	double sun_range_rate = 1000.0*solar_set.w;

	obs->time = time;
	obs->azimuth = sun_azi;
	obs->elevation = sun_ele;
	obs->range = sun_range;
	obs->range_rate = sun_range_rate;
}

/**
 * Calculate RA and dec for the sun.
 *
 * \param time Time
 * \param ra Right ascension
 * \param dec Declination
 * \copyright GPLv2+
 **/
void predict_sun_ra_dec(predict_julian_date_t time, double *ra, double *dec)
{
	//predict absolute position of the sun
	double solar_vector[3];
	sun_predict(time, solar_vector);

	//prepare for radec calculation
	double jul_utc = time + JULIAN_TIME_DIFF;
	double zero_vector[3] = {0,0,0};
	vector_t solar_rad;

	//for some reason, RADec requires QTH coordinates, though
	//the properties to be calculated are observer-independent.
	//Pick some coordinates, will be correct anyway.
	geodetic_t geodetic;
	geodetic.lat = 10;
	geodetic.lon = 10;
	geodetic.alt = 10 / 1000.0;
	geodetic.theta = 0.0;

	//calculate right ascension/declination
	Calculate_RADec(jul_utc, solar_vector, zero_vector, &geodetic, &solar_rad);
	*ra = solar_rad.x;
	*dec = solar_rad.y;
}

double predict_sun_ra(predict_julian_date_t time)
{
	double ra, dec;
	predict_sun_ra_dec(time, &ra, &dec);
	return ra;
}

double predict_sun_declination(predict_julian_date_t time)
{
	double ra, dec;
	predict_sun_ra_dec(time, &ra, &dec);
	return dec;
}

double predict_sun_gha(predict_julian_date_t time)
{
	//predict absolute position of sun
	double solar_vector[3];
	sun_predict(time, solar_vector);

	//convert to lat/lon/alt
	geodetic_t solar_latlonalt;
	Calculate_LatLonAlt(time, solar_vector, &solar_latlonalt);

	//return longitude as the GHA
	double sun_lon = 360.0-Degrees(solar_latlonalt.lon);
	return sun_lon*M_PI/180.0;
}
