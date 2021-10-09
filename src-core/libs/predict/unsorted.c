#include "unsorted.h"

#include "defs.h"

void vec3_set(double v[3], double x, double y, double z)
{
	v[0] = x;
	v[1] = y;
	v[2] = z;
}

double vec3_length(const double v[3])
{
	return sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
}

double vec3_dot(const double v[3], const double u[3])
{
	return (v[0]*u[0] + v[1]*u[1] + v[2]*u[2]);
}

void vec3_mul_scalar(const double v[3], double a, double r[3])
{
	r[0] = v[0]*a;
	r[1] = v[1]*a;
	r[2] = v[2]*a;
}

void vec3_sub(const double v1[3], const double v2[3], double *r)
{
	r[0] = v1[0] - v2[0];
	r[1] = v1[1] - v2[1];
	r[2] = v1[2] - v2[2];
}

double Sqr(double arg)
{
	/* Returns square of a double */
	return (arg*arg);
}

double FMod2p(double x)
{
	/* Returns mod 2PI of argument */

	double ret_val = fmod(x, 2*M_PI);

	if (ret_val < 0.0)
		ret_val += (2*M_PI);

	return ret_val;
}

void Convert_Sat_State(double pos[3], double vel[3])
{
	/* Converts the satellite's position and velocity  */
	/* vectors from normalized values to km and km/sec */ 

	vec3_mul_scalar(pos, EARTH_RADIUS_KM_WGS84, pos);
	vec3_mul_scalar(vel, EARTH_RADIUS_KM_WGS84*MINUTES_PER_DAY/SECONDS_PER_DAY, vel);
}

double Julian_Date_of_Year(double year)
{
	/* The function Julian_Date_of_Year calculates the Julian Date  */
	/* of Day 0.0 of {year}. This function is used to calculate the */
	/* Julian Date of any date by using Julian_Date_of_Year, DOY,   */
	/* and Fraction_of_Day. */

	/* Astronomical Formulae for Calculators, Jean Meeus, */
	/* pages 23-25. Calculate Julian Date of 0.0 Jan year */

	long A, B, i;
	double jdoy;

	year=year-1;
	i=year/100;
	A=i;
	i=A/4;
	B=2-A+i;
	i=365.25*year;
	i+=30.6001*14;
	jdoy=i+1720994.5+B;

	return jdoy;
}

double Julian_Date_of_Epoch(double epoch)
{ 
	/* The function Julian_Date_of_Epoch returns the Julian Date of     */
	/* an epoch specified in the format used in the NORAD two-line      */
	/* element sets. It has been modified to support dates beyond       */
	/* the year 1999 assuming that two-digit years in the range 00-56   */
	/* correspond to 2000-2056. Until the two-line element set format   */
	/* is changed, it is only valid for dates through 2056 December 31. */

	double year, day;

	/* Modification to support Y2K */
	/* Valid 1957 through 2056     */

	day=modf(epoch*1E-3, &year)*1E3;

	if (year<57)
		year=year+2000;
	else
		year=year+1900;

	return (Julian_Date_of_Year(year)+day);
}

double ThetaG_JD(double jd)
{
	/* Reference:  The 1992 Astronomical Almanac, page B6. */

	double UT, TU, GMST;

	double dummy;
	UT=modf(jd+0.5, &dummy);
	jd = jd - UT;
	TU=(jd-2451545.0)/36525;
	GMST=24110.54841+TU*(8640184.812866+TU*(0.093104-TU*6.2E-6));
	GMST=fmod(GMST+SECONDS_PER_DAY*EARTH_ROTATIONS_PER_SIDERIAL_DAY*UT,SECONDS_PER_DAY);

	return (2*M_PI*GMST/SECONDS_PER_DAY);
}

void Calculate_User_PosVel(double time, geodetic_t *geodetic, double obs_pos[3], double obs_vel[3])
{
	/* Calculate_User_PosVel() passes the user's geodetic position
	   and the time of interest and returns the ECI position and
	   velocity of the observer.  The velocity calculation assumes
	   the geodetic position is stationary relative to the earth's
	   surface. */

	/* Reference:  The 1992 Astronomical Almanac, page K11. */

	double c, sq, achcp;

	geodetic->theta=FMod2p(ThetaG_JD(time)+geodetic->lon); /* LMST */
	c=1/sqrt(1+FLATTENING_FACTOR*(FLATTENING_FACTOR-2)*Sqr(sin(geodetic->lat)));
	sq=Sqr(1-FLATTENING_FACTOR)*c;
	achcp=(EARTH_RADIUS_KM_WGS84*c+geodetic->alt)*cos(geodetic->lat);
	obs_pos[0] = (achcp*cos(geodetic->theta)); /* kilometers */
	obs_pos[1] = (achcp*sin(geodetic->theta));
	obs_pos[2] = ((EARTH_RADIUS_KM_WGS84*sq+geodetic->alt)*sin(geodetic->lat));
	obs_vel[0] = (-EARTH_ANGULAR_VELOCITY*obs_pos[1]); /* kilometers/second */
	obs_vel[1] = (EARTH_ANGULAR_VELOCITY*obs_pos[0]);
	obs_vel[2] = (0);
}

long DayNum(int m, int d, int y)
{

	long dn;
	double mm, yy;

	if (m<3) {
		y--;
		m+=12;
	}

	if (y<57)
		y+=100;

	yy=(double)y;
	mm=(double)m;
	dn=(long)(floor(365.25*(yy-80.0))-floor(19.0+yy/100.0)+floor(4.75+yy/400.0)-16.0);
	dn+=d+30*m+(long)floor(0.6*mm-0.3);
	return dn;
}


void Calculate_LatLonAlt(double time, const double pos[3],  geodetic_t *geodetic)
{
	/* Procedure Calculate_LatLonAlt will calculate the geodetic  */
	/* position of an object given its ECI position pos and time. */
	/* It is intended to be used to determine the ground track of */
	/* a satellite.  The calculations  assume the earth to be an  */
	/* oblate spheroid as defined in WGS '72.                     */

	/* Reference:  The 1992 Astronomical Almanac, page K12. */

	double r, e2, phi, c;
	
	//Convert to julian time:
	time += JULIAN_TIME_DIFF;

	geodetic->theta = atan2(pos[1], pos[0]); /* radians */
	geodetic->lon = FMod2p(geodetic->theta-ThetaG_JD(time)); /* radians */
	r = sqrt(Sqr(pos[0])+Sqr(pos[1]));
	e2 = FLATTENING_FACTOR*(2-FLATTENING_FACTOR);
	geodetic->lat=atan2(pos[2],r); /* radians */

	do
	{
		phi=geodetic->lat;
		c=1/sqrt(1-e2*Sqr(sin(phi)));
		geodetic->lat=atan2(pos[2]+EARTH_RADIUS_KM_WGS84*c*e2*sin(phi),r);

	} while (fabs(geodetic->lat-phi)>=1E-10);

	geodetic->alt=r/cos(geodetic->lat)-EARTH_RADIUS_KM_WGS84*c; /* kilometers */

	if (geodetic->lat>PI_HALF)
		geodetic->lat-= 2*M_PI;
}

void Calculate_Obs(double time, const double pos[3], const double vel[3], geodetic_t *geodetic, vector_t *obs_set)
{
	/* The procedures Calculate_Obs and Calculate_RADec calculate         */
	/* the *topocentric* coordinates of the object with ECI position,     */
	/* {pos}, and velocity, {vel}, from location {geodetic} at {time}.    */
	/* The {obs_set} returned for Calculate_Obs consists of azimuth,      */
	/* elevation, range, and range rate (in that order) with units of     */
	/* radians, radians, kilometers, and kilometers/second, respectively. */
	/* The WGS '72 geoid is used and the effect of atmospheric refraction */
	/* (under standard temperature and pressure) is incorporated into the */
	/* elevation calculation; the effect of atmospheric refraction on     */
	/* range and range rate has not yet been quantified.                  */

	/* The {obs_set} for Calculate_RADec consists of right ascension and  */
	/* declination (in that order) in radians.  Again, calculations are   */
	/* based on *topocentric* position using the WGS '72 geoid and        */
	/* incorporating atmospheric refraction.                              */

	double sin_lat, cos_lat, sin_theta, cos_theta, el, azim, top_s, top_e, top_z;

	double obs_pos[3];
	double obs_vel[3];
	double range[3];
	double rgvel[3];

	Calculate_User_PosVel(time, geodetic, obs_pos, obs_vel);

	vec3_sub(pos, obs_pos, range);
	vec3_sub(vel, obs_vel, rgvel);
	
	double range_length = vec3_length(range);

	sin_lat=sin(geodetic->lat);
	cos_lat=cos(geodetic->lat);
	sin_theta=sin(geodetic->theta);
	cos_theta=cos(geodetic->theta);
	top_s=sin_lat*cos_theta*range[0]+sin_lat*sin_theta*range[1]-cos_lat*range[2];
	top_e=-sin_theta*range[0]+cos_theta*range[1];
	top_z=cos_lat*cos_theta*range[0]+cos_lat*sin_theta*range[1]+sin_lat*range[2];
	azim=atan(-top_e/top_s); /* Azimuth */

	if (top_s>0.0) 
		azim=azim+PI;

	if (azim<0.0)
		azim = azim + 2*M_PI;

	el=asin_(top_z/range_length);
	obs_set->x=azim;	/* Azimuth (radians)   */
	obs_set->y=el;		/* Elevation (radians) */
	obs_set->z=range_length;	/* Range (kilometers)  */

	/* Range Rate (kilometers/second) */
	obs_set->w = vec3_dot(range, rgvel)/vec3_length(range);
	obs_set->y=el;

	/**** End bypass ****/

	if (obs_set->y<0.0)
		obs_set->y=el;  /* Reset to true elevation */
}

void Calculate_RADec(double time, const double pos[3], const double vel[3], geodetic_t *geodetic, vector_t *obs_set)
{
	/* Reference:  Methods of Orbit Determination by  */
	/*             Pedro Ramon Escobal, pp. 401-402   */

	double	phi, theta, sin_theta, cos_theta, sin_phi, cos_phi, az, el,
	Lxh, Lyh, Lzh, Sx, Ex, Zx, Sy, Ey, Zy, Sz, Ez, Zz, Lx, Ly,
	Lz, cos_delta, sin_alpha, cos_alpha;

	Calculate_Obs(time,pos,vel,geodetic,obs_set);

	az=obs_set->x;
	el=obs_set->y;
	phi=geodetic->lat;
	theta=FMod2p(ThetaG_JD(time)+geodetic->lon);
	sin_theta=sin(theta);
	cos_theta=cos(theta);
	sin_phi=sin(phi);
	cos_phi=cos(phi);
	Lxh=-cos(az)*cos(el);
	Lyh=sin(az)*cos(el);
	Lzh=sin(el);
	Sx=sin_phi*cos_theta;
	Ex=-sin_theta;
	Zx=cos_theta*cos_phi;
	Sy=sin_phi*sin_theta;
	Ey=cos_theta;
	Zy=sin_theta*cos_phi;
	Sz=-cos_phi;
	Ez=0.0;
	Zz=sin_phi;
	Lx=Sx*Lxh+Ex*Lyh+Zx*Lzh;
	Ly=Sy*Lxh+Ey*Lyh+Zy*Lzh;
	Lz=Sz*Lxh+Ez*Lyh+Zz*Lzh;
	obs_set->y=asin_(Lz);  /* Declination (radians) */
	cos_delta=sqrt(1.0-Sqr(Lz));
	sin_alpha=Ly/cos_delta;
	cos_alpha=Lx/cos_delta;
	obs_set->x=atan2(sin_alpha,cos_alpha); /* Right Ascension (radians) */
	obs_set->x=FMod2p(obs_set->x);
}

/* .... SGP4/SDP4 functions end .... */

char *SubString(const char *string, int buffer_length, char *output_buffer, int start, int end)
{

	unsigned x, y;

	if ((end >= start) && (buffer_length > end - start + 2))
	{
		for (x=start, y=0; x<=end && string[x]!=0; x++)
			if (string[x]!=' ')
			{
				output_buffer[y] = string[x];
				y++;
			}

		output_buffer[y]=0;
		return output_buffer;
	}
	else
		return NULL;
}

double acos_(double arg)
{
	return acos(arg < -1.0 ? -1.0 : (arg > 1.0 ? 1.0 : arg));
}

double asin_(double arg)
{
	return asin(arg < -1.0 ? -1.0 : (arg > 1.0 ? 1.0 : arg));
}
