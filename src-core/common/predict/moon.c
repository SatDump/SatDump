#include "predict.h"
#include "unsorted.h"
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "sun.h"

/**
 * This function reduces angles greater than two pi by subtracting two pi
 * from the angle
 *
 * \copyright GPLv2+
 **/
double FixAngle(double x)
{

	while ( x > 2*M_PI )
		x-=2*M_PI;

	return x;
}

/**
 * This function is used in the FindMoon() function.
 *
 * \copyright GPLv2+
 **/
double PrimeAngle(double x)
{
	x=x-360.0*floor(x/360.0);
	return x;
}

/**
 * Output struct for predict_moon().
 **/
struct moon {
	///Julian day
	double jd;
	///Related to ecliptic longitude
	double lm;
	///Related to ecliptic latitude
	double b;
	///Parallax-related
	double p;
	///Related to siderial time
	double teg;
	///Range approximation?
	double dx;
};

/**
 * Predict absolute, observer-independent properties of the moon.
 *
 * \param time Time
 * \param moon Output struct
 * \copyright GPLv2+
 **/
void predict_moon(double time, struct moon *moon)
{
	double jd, t, t2, t3, l1, m, teg, l, b, w1, w2, bt, p, lm, m1, d, ff, om, ss, ex;


	jd = time + JULIAN_TIME_DIFF;
	moon->jd = jd;

	t=(jd-2415020.0)/36525.0;
	t2=t*t;
	t3=t2*t;
	l1=270.434164+481267.8831*t-0.001133*t2+0.0000019*t3;
	m=358.475833+35999.0498*t-0.00015*t2-0.0000033*t3;
	m1=296.104608+477198.8491*t+0.009192*t2+0.0000144*t3;
	d=350.737486+445267.1142*t-0.001436*t2+0.0000019*t3;
	ff=11.250889+483202.0251*t-0.003211*t2-0.0000003*t3;
	om=259.183275-1934.142*t+0.002078*t2+0.0000022*t3;
	om = om * M_PI/180.0;

	/* Additive terms */

	l1=l1+0.000233*sin((51.2+20.2*t)*M_PI/180.0);
	ss=0.003964*sin((346.56+132.87*t-0.0091731*t2)*M_PI/180.0);
	l1=l1+ss+0.001964*sin(om);
	m=m-0.001778*sin((51.2+20.2*t)*M_PI/180.0);
	m1=m1+0.000817*sin((51.2+20.2*t)*M_PI/180.0);
	m1=m1+ss+0.002541*sin(om);
	d=d+0.002011*sin((51.2+20.2*t)*M_PI/180.0);
	d=d+ss+0.001964*sin(om);
	ff=ff+ss-0.024691*sin(om);
	ff=ff-0.004328*sin(om+(275.05-2.3*t)*M_PI/180.0);
	ex=1.0-0.002495*t-0.00000752*t2;
	om=om*M_PI/180.0;

	l1=PrimeAngle(l1);
	m=PrimeAngle(m);
	m1=PrimeAngle(m1);
	d=PrimeAngle(d);
	ff=PrimeAngle(ff);
	om=PrimeAngle(om);

	m=m*M_PI/180.0;
	m1=m1*M_PI/180.0;
	d=d*M_PI/180.0;
	ff=ff*M_PI/180.0;

	/* Ecliptic Longitude */

	l=l1+6.28875*sin(m1)+1.274018*sin(2.0*d-m1)+0.658309*sin(2.0*d);
	l=l+0.213616*sin(2.0*m1)-ex*0.185596*sin(m)-0.114336*sin(2.0*ff);
	l=l+0.058793*sin(2.0*d-2.0*m1)+ex*0.057212*sin(2.0*d-m-m1)+0.05332*sin(2.0*d+m1);
	l=l+ex*0.045874*sin(2.0*d-m)+ex*0.041024*sin(m1-m)-0.034718*sin(d);
	l=l-ex*0.030465*sin(m+m1)+0.015326*sin(2.0*d-2.0*ff)-0.012528*sin(2.0*ff+m1);

	l=l-0.01098*sin(2.0*ff-m1)+0.010674*sin(4.0*d-m1)+0.010034*sin(3.0*m1);
	l=l+0.008548*sin(4.0*d-2.0*m1)-ex*0.00791*sin(m-m1+2.0*d)-ex*0.006783*sin(2.0*d+m);

	l=l+0.005162*sin(m1-d)+ex*0.005*sin(m+d)+ex*0.004049*sin(m1-m+2.0*d);
	l=l+0.003996*sin(2.0*m1+2.0*d)+0.003862*sin(4.0*d)+0.003665*sin(2.0*d-3.0*m1);

	l=l+ex*0.002695*sin(2.0*m1-m)+0.002602*sin(m1-2.0*ff-2.0*d)+ex*0.002396*sin(2.0*d-m-2.0*m1);

	l=l-0.002349*sin(m1+d)+ex*ex*0.002249*sin(2.0*d-2.0*m)-ex*0.002125*sin(2.0*m1+m);

	l=l-ex*ex*0.002079*sin(2.0*m)+ex*ex*0.002059*sin(2.0*d-m1-2.0*m)-0.001773*sin(m1+2.0*d-2.0*ff);

	l=l+ex*0.00122*sin(4.0*d-m-m1)-0.00111*sin(2.0*m1+2.0*ff)+0.000892*sin(m1-3.0*d);

	l=l-ex*0.000811*sin(m+m1+2.0*d)+ex*0.000761*sin(4.0*d-m-2.0*m1)+ex*ex*.000717*sin(m1-2.0*m);

	l=l+ex*ex*0.000704*sin(m1-2.0*m-2.0*d)+ex*0.000693*sin(m-2.0*m1+2.0*d)+ex*0.000598*sin(2.0*d-m-2.0*ff)+0.00055*sin(m1+4.0*d);

	l=l+0.000538*sin(4.0*m1)+ex*0.000521*sin(4.0*d-m)+0.000486*sin(2.0*m1-d);

	l=l-0.001595*sin(2.0*ff+2.0*d);

	/* Ecliptic latitude */

	b=5.128189*sin(ff)+0.280606*sin(m1+ff)+0.277693*sin(m1-ff)+0.173238*sin(2.0*d-ff);
	b=b+0.055413*sin(2.0*d+ff-m1)+0.046272*sin(2.0*d-ff-m1)+0.032573*sin(2.0*d+ff);

	b=b+0.017198*sin(2.0*m1+ff)+9.266999e-03*sin(2.0*d+m1-ff)+0.008823*sin(2.0*m1-ff);
	b=b+ex*0.008247*sin(2.0*d-m-ff)+0.004323*sin(2.0*d-ff-2.0*m1)+0.0042*sin(2.0*d+ff+m1);

	b=b+ex*0.003372*sin(ff-m-2.0*d)+ex*0.002472*sin(2.0*d+ff-m-m1)+ex*0.002222*sin(2.0*d+ff-m);

	b=b+0.002072*sin(2.0*d-ff-m-m1)+ex*0.001877*sin(ff-m+m1)+0.001828*sin(4.0*d-ff-m1);

	b=b-ex*0.001803*sin(ff+m)-0.00175*sin(3.0*ff)+ex*0.00157*sin(m1-m-ff)-0.001487*sin(ff+d)-ex*0.001481*sin(ff+m+m1)+ex*0.001417*sin(ff-m-m1)+ex*0.00135*sin(ff-m)+0.00133*sin(ff-d);

	b=b+0.001106*sin(ff+3.0*m1)+0.00102*sin(4.0*d-ff)+0.000833*sin(ff+4.0*d-m1);

	b=b+0.000781*sin(m1-3.0*ff)+0.00067*sin(ff+4.0*d-2.0*m1)+0.000606*sin(2.0*d-3.0*ff);

	b=b+0.000597*sin(2.0*d+2.0*m1-ff)+ex*0.000492*sin(2.0*d+m1-m-ff)+0.00045*sin(2.0*m1-ff-2.0*d);

	b=b+0.000439*sin(3.0*m1-ff)+0.000423*sin(ff+2.0*d+2.0*m1)+0.000422*sin(2.0*d-ff-3.0*m1);

	b=b-ex*0.000367*sin(m+ff+2.0*d-m1)-ex*0.000353*sin(m+ff+2.0*d)+0.000331*sin(ff+4.0*d);

	b=b+ex*0.000317*sin(2.0*d+ff-m+m1)+ex*ex*0.000306*sin(2.0*d-2.0*m-ff)-0.000283*sin(m1+3.0*ff);

	w1=0.0004664*cos(om*M_PI/180.0);
	w2=0.0000754*cos((om+275.05-2.3*t)*M_PI/180.0);
	bt=b*(1.0-w1-w2);

	/* Parallax calculations */

	p=0.950724+0.051818*cos(m1)+0.009531*cos(2.0*d-m1)+0.007843*cos(2.0*d)+0.002824*cos(2.0*m1)+0.000857*cos(2.0*d+m1)+ex*0.000533*cos(2.0*d-m)+ex*0.000401*cos(2.0*d-m-m1);

	p=p+0.000173*cos(3.0*m1)+0.000167*cos(4.0*d-m1)-ex*0.000111*cos(m)+0.000103*cos(4.0*d-2.0*m1)-0.000084*cos(2.0*m1-2.0*d)-ex*0.000083*cos(2.0*d+m)+0.000079*cos(2.0*d+2.0*m1);

	p=p+0.000072*cos(4.0*d)+ex*0.000064*cos(2.0*d-m+m1)-ex*0.000063*cos(2.0*d+m-m1);

	p=p+ex*0.000041*cos(m+d)+ex*0.000035*cos(2.0*m1-m)-0.000033*cos(3.0*m1-2.0*d);

	p=p-0.00003*cos(m1+d)-0.000029*cos(2.0*ff-2.0*d)-ex*0.000029*cos(2.0*m1+m);

	p=p+ex*ex*0.000026*cos(2.0*d-2.0*m)-0.000023*cos(2.0*ff-2.0*d+m1)+ex*0.000019*cos(4.0*d-m-m1);

	b=bt*M_PI/180.0;
	lm=l*M_PI/180.0;

	/* Find siderial time in radians */

	t=(jd-2451545.0)/36525.0;
	teg=280.46061837+360.98564736629*(jd-2451545.0)+(0.000387933*t-t*t/38710000.0)*t;

	while (teg>360.0)
		teg-=360.0;

	//output
	moon->b = b;
	moon->lm = lm;
	moon->p = p;
	moon->dx = 3.0/(M_PI*p);
	moon->teg = teg;
}

/**
 * Calculate RA and dec for the moon.
 *
 * \param time Time
 * \param ra Right ascension
 * \param dec Declination
 * \copyright GPLv2+
 **/
void predict_moon_ra_dec(predict_julian_date_t time, double *ra, double *dec)
{
	struct moon moon;
	predict_moon(time, &moon);

	/* Semi-diameter calculation */
	/* sem=10800.0*asin(0.272488*p*M_PI/180.0)/pi; */
	/* Convert ecliptic coordinates to equatorial coordinates */

	double z=(moon.jd-2415020.5)/365.2422;
	double ob=23.452294-(0.46845*z+5.9e-07*z*z)/3600.0;
	ob=ob*M_PI/180.0;
	*dec=asin(sin(moon.b)*cos(ob)+cos(moon.b)*sin(ob)*sin(moon.lm));
	*ra=acos(cos(moon.b)*cos(moon.lm)/cos(*dec));

	if (moon.lm > M_PI)
		*ra = 2*M_PI - *ra;
}

void predict_observe_moon(const predict_observer_t *observer, double time, struct predict_observation *obs)
{
	struct moon moon;
	predict_moon(time, &moon);

	double ra, dec;
	predict_moon_ra_dec(time, &ra, &dec);

	double n = observer->latitude;    /* North latitude of tracking station */
	double e = observer->longitude;  /* East longitude of tracking station */


	double th = FixAngle(moon.teg*M_PI/180.0 + e);
	double h=th-ra;

	double az=atan2(sin(h),cos(h)*sin(n)-tan(dec)*cos(n))+M_PI;
	double el=asin(sin(n)*sin(dec)+cos(n)*cos(dec)*cos(h));

	/* Radial velocity approximation.  This code was derived
	   from "Amateur Radio Software", by John Morris, GM4ANB,
	   published by the RSGB in 1985. */

	double mm=FixAngle(1.319238+time*0.228027135);  /* mean moon position */
	double t2=0.10976;
	double t1=mm+t2*sin(mm);
	double dv=0.01255*moon.dx*moon.dx*sin(t1)*(1.0+t2*cos(mm));
	dv=dv*4449.0;
	t1=6378.0;
	t2=384401.0;
	double t3=t1*t2*(cos(dec)*cos(n)*sin(h));
	t3=t3/sqrt(t2*t2-t2*t1*sin(el));

	double moon_dv=dv+t3*0.0753125;

	obs->time = time;
	obs->azimuth = az;
	obs->elevation = el;
	obs->range = moon.dx;
	obs->range_rate = moon_dv;
}

double predict_moon_ra(predict_julian_date_t time)
{
	double ra, dec;
	predict_moon_ra_dec(time, &ra, &dec);
	return ra;
}

double predict_moon_declination(predict_julian_date_t time)
{
	double ra, dec;
	predict_moon_ra_dec(time, &ra, &dec);
	return dec;
}

double predict_moon_gha(predict_julian_date_t time)
{
	struct moon moon;
	predict_moon(time, &moon);
	double moon_gha=moon.teg-predict_moon_ra(time)*180.0/M_PI;

	if (moon_gha<0.0) moon_gha+=360;
	return moon_gha*M_PI/180.0;
}
