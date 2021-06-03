#include "sdp4.h"

#include <math.h>
#include <stdbool.h>

#include "defs.h"
#include "unsorted.h"

/// Entry points of deep()
#define DPSecular	1
#define DPPeriodic	2

/**
 * Initialize the fixed part of deep_arg.
 *
 * \param tle Orbital elements
 * \param m Initialized sdp4 model
 * \param deep_arg Fixed part of deep_arg
 * \copyright GPLv2+
 **/
void sdp4_deep_initialize(const predict_orbital_elements_t *tle, struct _sdp4 *m, deep_arg_fixed_t *deep_arg);

/**
 * Initialize the dynamic part of deep_arg.
 *
 * \param m Initialized sdp4 model
 * \param deep_dyn Dynamic part of deep_arg
 * \copyright GPLv2+
 **/
void deep_arg_dynamic_init(const struct _sdp4 *m, deep_arg_dynamic_t *deep_dyn);

void sdp4_init(const predict_orbital_elements_t *tle, struct _sdp4 *m)
{
	m->lunarTermsDone = 0;
	m->resonanceFlag = 0;
	m->synchronousFlag = 0;

	//Calculate old TLE field values as used in the original sdp4
	double temp_tle = TWO_PI/MINUTES_PER_DAY/MINUTES_PER_DAY;
	m->xnodeo = tle->right_ascension * M_PI / 180.0;
	m->omegao = tle->argument_of_perigee * M_PI / 180.0;
	m->xmo = tle->mean_anomaly * M_PI / 180.0;
	m->xincl = tle->inclination * M_PI / 180.0;
	m->eo = tle->eccentricity;
	m->xno = tle->mean_motion*temp_tle*MINUTES_PER_DAY;
	m->bstar = tle->bstar_drag_term / AE;
	m->epoch = 1000.0*tle->epoch_year + tle->epoch_day;

	/* Recover original mean motion (xnodp) and   */
	/* semimajor axis (aodp) from input elements. */
	double temp1, temp2, temp3, theta4, a1, a3ovk2, ao, c2, coef, coef1, x1m5th, xhdot1, del1, delo, eeta, eta, etasq, perigee, psisq, tsi, qoms24, s4, pinvsq;

	a1=pow(XKE/m->xno,TWO_THIRD);
	m->deep_arg.cosio=cos(m->xincl);
	m->deep_arg.theta2=m->deep_arg.cosio*m->deep_arg.cosio;
	m->x3thm1=3*m->deep_arg.theta2-1;
	m->deep_arg.eosq=m->eo*m->eo;
	m->deep_arg.betao2=1-m->deep_arg.eosq;
	m->deep_arg.betao=sqrt(m->deep_arg.betao2);
	del1=1.5*CK2*m->x3thm1/(a1*a1*m->deep_arg.betao*m->deep_arg.betao2);
	ao=a1*(1-del1*(0.5*TWO_THIRD+del1*(1+134/81*del1)));
	delo=1.5*CK2*m->x3thm1/(ao*ao*m->deep_arg.betao*m->deep_arg.betao2);
	m->deep_arg.xnodp=m->xno/(1+delo);
	m->deep_arg.aodp=ao/(1-delo);

	/* For perigee below 156 km, the values */
	/* of s and qoms2t are altered.         */

	s4=S_DENSITY_PARAM;
	qoms24=QOMS2T;
	perigee=(m->deep_arg.aodp*(1-m->eo)-AE)*EARTH_RADIUS_KM_WGS84;

	if (perigee<156.0)
	{
		if (perigee<=98.0)
			s4=20.0;
		else
			s4=perigee-78.0;

		qoms24=pow((120-s4)*AE/EARTH_RADIUS_KM_WGS84,4);
		s4=s4/EARTH_RADIUS_KM_WGS84+AE;
	}

	pinvsq=1/(m->deep_arg.aodp*m->deep_arg.aodp*m->deep_arg.betao2*m->deep_arg.betao2);
	m->deep_arg.sing=sin(m->omegao);
	m->deep_arg.cosg=cos(m->omegao);
	tsi=1/(m->deep_arg.aodp-s4);
	eta=m->deep_arg.aodp*m->eo*tsi;
	etasq=eta*eta;
	eeta=m->eo*eta;
	psisq=fabs(1-etasq);
	coef=qoms24*pow(tsi,4);
	coef1=coef/pow(psisq,3.5);
	c2=coef1*m->deep_arg.xnodp*(m->deep_arg.aodp*(1+1.5*etasq+eeta*(4+etasq))+0.75*CK2*tsi/psisq*m->x3thm1*(8+3*etasq*(8+etasq)));
	m->c1=m->bstar*c2;
	m->deep_arg.sinio=sin(m->xincl);
	a3ovk2=-J3_HARMONIC_WGS72/CK2*pow(AE,3);
	m->x1mth2=1-m->deep_arg.theta2;
	m->c4=2*m->deep_arg.xnodp*coef1*m->deep_arg.aodp*m->deep_arg.betao2*(eta*(2+0.5*etasq)+m->eo*(0.5+2*etasq)-2*CK2*tsi/(m->deep_arg.aodp*psisq)*(-3*m->x3thm1*(1-2*eeta+etasq*(1.5-0.5*eeta))+0.75*m->x1mth2*(2*etasq-eeta*(1+etasq))*cos(2*m->omegao)));
	theta4=m->deep_arg.theta2*m->deep_arg.theta2;
	temp1=3*CK2*pinvsq*m->deep_arg.xnodp;
	temp2=temp1*CK2*pinvsq;
	temp3=1.25*CK4*pinvsq*pinvsq*m->deep_arg.xnodp;
	m->deep_arg.xmdot=m->deep_arg.xnodp+0.5*temp1*m->deep_arg.betao*m->x3thm1+0.0625*temp2*m->deep_arg.betao*(13-78*m->deep_arg.theta2+137*theta4);
	x1m5th=1-5*m->deep_arg.theta2;
	m->deep_arg.omgdot=-0.5*temp1*x1m5th+0.0625*temp2*(7-114*m->deep_arg.theta2+395*theta4)+temp3*(3-36*m->deep_arg.theta2+49*theta4);
	xhdot1=-temp1*m->deep_arg.cosio;
	m->deep_arg.xnodot=xhdot1+(0.5*temp2*(4-19*m->deep_arg.theta2)+2*temp3*(3-7*m->deep_arg.theta2))*m->deep_arg.cosio;
	m->xnodcf=3.5*m->deep_arg.betao2*xhdot1*m->c1;
	m->t2cof=1.5*m->c1;
	m->xlcof=0.125*a3ovk2*m->deep_arg.sinio*(3+5*m->deep_arg.cosio)/(1+m->deep_arg.cosio);
	m->aycof=0.25*a3ovk2*m->deep_arg.sinio;
	m->x7thm1=7*m->deep_arg.theta2-1;

	/* initialize Deep() */
	sdp4_deep_initialize(tle, m, &(m->deep_arg));
}

void sdp4_predict(const struct _sdp4 *m, double tsince, struct model_output *output)
{

	int i;
	double a, axn, ayn, aynl, beta, betal, capu, cos2u, cosepw, cosik,
	cosnok, cosu, cosuk, ecose, elsq, epw, esine, pl,
	rdot,
	rdotk, rfdot, rfdotk, rk, sin2u, sinepw, sinik, sinnok, sinu,
	sinuk, tempe, templ, tsq, u, uk, ux, uy, uz, vx, vy, vz, xl,
	xlt, xmam, xmdf, xmx, xmy, xnoddf, xll,
	r,
	temp, tempa, temp1,
	temp2, temp3, temp4, temp5, temp6;
	double xnodek, xinck;

	/* Initialize dynamic part of deep_arg */
	deep_arg_dynamic_t deep_dyn;
	deep_arg_dynamic_init(m, &deep_dyn);

	/* Update for secular gravity and atmospheric drag */
	xmdf=m->xmo+m->deep_arg.xmdot*tsince;
	deep_dyn.omgadf=m->omegao+m->deep_arg.omgdot*tsince;
	xnoddf=m->xnodeo+m->deep_arg.xnodot*tsince;
	tsq=tsince*tsince;
	deep_dyn.xnode=xnoddf+m->xnodcf*tsq;
	tempa=1-m->c1*tsince;
	tempe=m->bstar*m->c4*tsince;
	templ=m->t2cof*tsq;
	deep_dyn.xn=m->deep_arg.xnodp;

	/* Update for deep-space secular effects */
	deep_dyn.xll=xmdf;
	deep_dyn.t=tsince;

	sdp4_deep(m, DPSecular, &m->deep_arg, &deep_dyn);

	xmdf=deep_dyn.xll;
	a=pow(XKE/deep_dyn.xn,TWO_THIRD)*tempa*tempa;
	deep_dyn.em=deep_dyn.em-tempe;
	xmam=xmdf+m->deep_arg.xnodp*templ;

	/* Update for deep-space periodic effects */
	deep_dyn.xll=xmam;

	sdp4_deep(m, DPPeriodic,&m->deep_arg, &deep_dyn);

	xmam=deep_dyn.xll;
	xl=xmam+deep_dyn.omgadf+deep_dyn.xnode;
	beta=sqrt(1-deep_dyn.em*deep_dyn.em);
	deep_dyn.xn=XKE/pow(a,1.5);

	/* Long period periodics */
	axn=deep_dyn.em*cos(deep_dyn.omgadf);
	temp=1/(a*beta*beta);
	xll=temp*m->xlcof*axn;
	aynl=temp*m->aycof;
	xlt=xl+xll;
	ayn=deep_dyn.em*sin(deep_dyn.omgadf)+aynl;

	/* Solve Kepler's Equation */
	capu=FMod2p(xlt-deep_dyn.xnode);
	temp2=capu;
	i=0;

	do
	{
		sinepw=sin(temp2);
		cosepw=cos(temp2);
		temp3=axn*sinepw;
		temp4=ayn*cosepw;
		temp5=axn*cosepw;
		temp6=ayn*sinepw;
		epw=(capu-temp4+temp3-temp2)/(1-temp5-temp6)+temp2;

		if (fabs(epw-temp2)<=E6A)
			break;

		temp2=epw;

	} while (i++<10);

	/* Short period preliminary quantities */
	ecose=temp5+temp6;
	esine=temp3-temp4;
	elsq=axn*axn+ayn*ayn;
	temp=1-elsq;
	pl=a*temp;
	r=a*(1-ecose);
	temp1=1/r;
	rdot=XKE*sqrt(a)*esine*temp1;
	rfdot=XKE*sqrt(pl)*temp1;
	temp2=a*temp1;
	betal=sqrt(temp);
	temp3=1/(1+betal);
	cosu=temp2*(cosepw-axn+ayn*esine*temp3);
	sinu=temp2*(sinepw-ayn-axn*esine*temp3);
	u=atan2(sinu,cosu);
	sin2u=2*sinu*cosu;
	cos2u=2*cosu*cosu-1;
	temp=1/pl;
	temp1=CK2*temp;
	temp2=temp1*temp;

	/* Update for short periodics */
	rk=r*(1-1.5*temp2*betal*m->x3thm1)+0.5*temp1*m->x1mth2*cos2u;
	uk=u-0.25*temp2*m->x7thm1*sin2u;
	xnodek=deep_dyn.xnode+1.5*temp2*m->deep_arg.cosio*sin2u;
	xinck=deep_dyn.xinc+1.5*temp2*m->deep_arg.cosio*m->deep_arg.sinio*cos2u;
	rdotk=rdot-deep_dyn.xn*temp1*m->x1mth2*sin2u;
	rfdotk=rfdot+deep_dyn.xn*temp1*(m->x1mth2*cos2u+1.5*m->x3thm1);

	/* Orientation vectors */
	sinuk=sin(uk);
	cosuk=cos(uk);
	sinik=sin(xinck);
	cosik=cos(xinck);
	sinnok=sin(xnodek);
	cosnok=cos(xnodek);
	xmx=-sinnok*cosik;
	xmy=cosnok*cosik;
	ux=xmx*sinuk+cosnok*cosuk;
	uy=xmy*sinuk+sinnok*cosuk;
	uz=sinik*sinuk;
	vx=xmx*cosuk-cosnok*sinuk;
	vy=xmy*cosuk-sinnok*sinuk;
	vz=sinik*cosuk;

	/* Position and velocity */
	output->pos[0] = rk*ux;
	output->pos[1] = rk*uy;
	output->pos[2] = rk*uz;
	output->vel[0] = rdotk*ux+rfdotk*vx;
	output->vel[1] = rdotk*uy+rfdotk*vy;
	output->vel[2] = rdotk*uz+rfdotk*vz;

	/* Phase in radians */
	double phase=xlt-deep_dyn.xnode-deep_dyn.omgadf+TWO_PI;

	if (phase<0.0)
		phase+=TWO_PI;

	phase=FMod2p(phase);
	output->phase = phase;

	output->omgadf = deep_dyn.omgadf;
	output->xnodek = xnodek;
	output->xinck = xinck;
}

/**
 * Calculates the Greenwich Mean Sidereal Time
 * for an epoch specified in the format used in the NORAD two-line
 * element sets.
 * It has been adapted for dates beyond the year 1999.
 * Reference:  The 1992 Astronomical Almanac, page B6.
 * Modification to support Y2K. Valid 1957 through 2056.
 *
 * \param epoch TLE epoch
 * \param deep_arg Deep arg
 * \copyright GPLv2+
 **/
double ThetaG(double epoch, deep_arg_fixed_t *deep_arg)
{
	double year, day, UT, jd, TU, GMST, ThetaG;

	/* Modification to support Y2K */
	/* Valid 1957 through 2056     */

	day=modf(epoch*1E-3,&year)*1E3;

	if (year<57)
		year+=2000;
	else
		year+=1900;

	UT=modf(day,&day);
	jd=Julian_Date_of_Year(year)+day;
	TU=(jd-2451545.0)/36525;
	GMST=24110.54841+TU*(8640184.812866+TU*(0.093104-TU*6.2E-6));
	GMST=fmod(GMST+SECONDS_PER_DAY*EARTH_ROTATIONS_PER_SIDERIAL_DAY*UT,SECONDS_PER_DAY);
	ThetaG = 2*M_PI*GMST/SECONDS_PER_DAY;
	deep_arg->ds50=jd-2433281.5+UT;
	ThetaG=FMod2p(6.3003880987*deep_arg->ds50+1.72944494);

	return ThetaG;
}

void sdp4_deep_initialize(const predict_orbital_elements_t *tle, struct _sdp4 *m, deep_arg_fixed_t *deep_arg)
{
	double a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, ainv2, aqnv,
	sgh, sini2, sh, si, day, bfact, c,
	cc, cosq, ctem, f322, zx, zy, eoc, eq,
	f220, f221, f311, f321, f330, f441, f442, f522, f523,
	f542, f543, g200, g201, g211, s1, s2, s3, s4, s5, s6, s7,
	se, g300, g310, g322, g410, g422, g520, g521, g532,
	g533, gam, sinq, sl, stem, temp, temp1, x1,
	x2, x3, x4, x5, x6, x7, x8, xmao,
	xno2, xnodce, xnoi, xpidot, z1, z11, z12, z13, z2,
	z21, z22, z23, z3, z31, z32, z33, ze, zn, zsing,
	zsinh, zsini, zcosg, zcosh, zcosi;

	/* Entrance for deep space initialization */
	m->thgr=ThetaG(m->epoch,deep_arg);
	eq=m->eo;
	m->xnq=deep_arg->xnodp;
	aqnv=1/deep_arg->aodp;
	m->xqncl=m->xincl;
	xmao=m->xmo;
	xpidot=deep_arg->omgdot+deep_arg->xnodot;
	sinq=sin(m->xnodeo);
	cosq=cos(m->xnodeo);
	m->omegaq=m->omegao;

	/* Initialize lunar solar terms */
	day=deep_arg->ds50+18261.5;  /* Days since 1900 Jan 0.5 */

	m->preep=day;
	xnodce=4.5236020-9.2422029E-4*day;
	stem=sin(xnodce);
	ctem=cos(xnodce);
	m->zcosil=0.91375164-0.03568096*ctem;
	m->zsinil=sqrt(1-m->zcosil*m->zcosil);
	m->zsinhl=0.089683511*stem/m->zsinil;
	m->zcoshl=sqrt(1-m->zsinhl*m->zsinhl);
	c=4.7199672+0.22997150*day;
	gam=5.8351514+0.0019443680*day;
	m->zmol=FMod2p(c-gam);
	zx=0.39785416*stem/m->zsinil;
	zy=m->zcoshl*ctem+0.91744867*m->zsinhl*stem;
	zx=atan2(zx,zy);
	zx=gam+zx-xnodce;
	m->zcosgl=cos(zx);
	m->zsingl=sin(zx);
	m->zmos=6.2565837+0.017201977*day;
	m->zmos=FMod2p(m->zmos);

	/* Do solar terms */
	zcosg=ZCOSGS;
	zsing=ZSINGS;
	zcosi=ZCOSIS;
	zsini=ZSINIS;
	zcosh=cosq;
	zsinh= sinq;
	cc=C1SS;
	zn=ZNS;
	ze=ZES;
	/* zmo=m->zmos; */
	xnoi=1/m->xnq;

	/* Loop breaks when Solar terms are done a second */
	/* time, after Lunar terms are initialized        */

	for (;;)
	{
		/* Solar terms done again after Lunar terms are done */
		a1=zcosg*zcosh+zsing*zcosi*zsinh;
		a3=-zsing*zcosh+zcosg*zcosi*zsinh;
		a7=-zcosg*zsinh+zsing*zcosi*zcosh;
		a8=zsing*zsini;
		a9=zsing*zsinh+zcosg*zcosi*zcosh;
		a10=zcosg*zsini;
		a2=deep_arg->cosio*a7+deep_arg->sinio*a8;
		a4=deep_arg->cosio*a9+deep_arg->sinio*a10;
		a5=-deep_arg->sinio*a7+deep_arg->cosio*a8;
		a6=-deep_arg->sinio*a9+deep_arg->cosio*a10;
		x1=a1*deep_arg->cosg+a2*deep_arg->sing;
		x2=a3*deep_arg->cosg+a4*deep_arg->sing;
		x3=-a1*deep_arg->sing+a2*deep_arg->cosg;
		x4=-a3*deep_arg->sing+a4*deep_arg->cosg;
		x5=a5*deep_arg->sing;
		x6=a6*deep_arg->sing;
		x7=a5*deep_arg->cosg;
		x8=a6*deep_arg->cosg;
		z31=12*x1*x1-3*x3*x3;
		z32=24*x1*x2-6*x3*x4;
		z33=12*x2*x2-3*x4*x4;
		z1=3*(a1*a1+a2*a2)+z31*deep_arg->eosq;
		z2=6*(a1*a3+a2*a4)+z32*deep_arg->eosq;
		z3=3*(a3*a3+a4*a4)+z33*deep_arg->eosq;
		z11=-6*a1*a5+deep_arg->eosq*(-24*x1*x7-6*x3*x5);
		z12=-6*(a1*a6+a3*a5)+deep_arg->eosq*(-24*(x2*x7+x1*x8)-6*(x3*x6+x4*x5));
		z13=-6*a3*a6+deep_arg->eosq*(-24*x2*x8-6*x4*x6);
		z21=6*a2*a5+deep_arg->eosq*(24*x1*x5-6*x3*x7);
		z22=6*(a4*a5+a2*a6)+deep_arg->eosq*(24*(x2*x5+x1*x6)-6*(x4*x7+x3*x8));
		z23=6*a4*a6+deep_arg->eosq*(24*x2*x6-6*x4*x8);
		z1=z1+z1+deep_arg->betao2*z31;
		z2=z2+z2+deep_arg->betao2*z32;
		z3=z3+z3+deep_arg->betao2*z33;
		s3=cc*xnoi;
		s2=-0.5*s3/deep_arg->betao;
		s4=s3*deep_arg->betao;
		s1=-15*eq*s4;
		s5=x1*x3+x2*x4;
		s6=x2*x3+x1*x4;
		s7=x2*x4-x1*x3;
		se=s1*zn*s5;
		si=s2*zn*(z11+z13);
		sl=-zn*s3*(z1+z3-14-6*deep_arg->eosq);
		sgh=s4*zn*(z31+z33-6);
		sh=-zn*s2*(z21+z23);

		if (m->xqncl<5.2359877E-2)
			sh=0;

		m->ee2=2*s1*s6;
		m->e3=2*s1*s7;
		m->xi2=2*s2*z12;
		m->xi3=2*s2*(z13-z11);
		m->xl2=-2*s3*z2;
		m->xl3=-2*s3*(z3-z1);
		m->xl4=-2*s3*(-21-9*deep_arg->eosq)*ze;
		m->xgh2=2*s4*z32;
		m->xgh3=2*s4*(z33-z31);
		m->xgh4=-18*s4*ze;
		m->xh2=-2*s2*z22;
		m->xh3=-2*s2*(z23-z21);

		//Skip lunar terms?
		if (m->lunarTermsDone) {
			break;
		}

		/* Do lunar terms */
		m->sse=se;
		m->ssi=si;
		m->ssl=sl;
		m->ssh=sh/deep_arg->sinio;
		m->ssg=sgh-deep_arg->cosio*m->ssh;
		m->se2=m->ee2;
		m->si2=m->xi2;
		m->sl2=m->xl2;
		m->sgh2=m->xgh2;
		m->sh2=m->xh2;
		m->se3=m->e3;
		m->si3=m->xi3;
		m->sl3=m->xl3;
		m->sgh3=m->xgh3;
		m->sh3=m->xh3;
		m->sl4=m->xl4;
		m->sgh4=m->xgh4;
		zcosg=m->zcosgl;
		zsing=m->zsingl;
		zcosi=m->zcosil;
		zsini=m->zsinil;
		zcosh=m->zcoshl*cosq+m->zsinhl*sinq;
		zsinh=sinq*m->zcoshl-cosq*m->zsinhl;
		zn=ZNL;
		cc=C1L;
		ze=ZEL;
		/* zmo=m->zmol; */
		//Set lunarTermsDone flag:
		m->lunarTermsDone = true;
	}

	m->sse=m->sse+se;
	m->ssi=m->ssi+si;
	m->ssl=m->ssl+sl;
	m->ssg=m->ssg+sgh-deep_arg->cosio/deep_arg->sinio*sh;
	m->ssh=m->ssh+sh/deep_arg->sinio;

	/* Geopotential resonance initialization for 12 hour orbits */
	m->resonanceFlag = 0;
	m->synchronousFlag = 0;

	if (!((m->xnq<0.0052359877) && (m->xnq>0.0034906585)))
	{
		if ((m->xnq<0.00826) || (m->xnq>0.00924))
		    return;

		if (eq<0.5)
		    return;

		m->resonanceFlag = 1;
		eoc=eq*deep_arg->eosq;
		g201=-0.306-(eq-0.64)*0.440;

		if (eq<=0.65)
		{
			g211=3.616-13.247*eq+16.290*deep_arg->eosq;
			g310=-19.302+117.390*eq-228.419*deep_arg->eosq+156.591*eoc;
			g322=-18.9068+109.7927*eq-214.6334*deep_arg->eosq+146.5816*eoc;
			g410=-41.122+242.694*eq-471.094*deep_arg->eosq+313.953*eoc;
			g422=-146.407+841.880*eq-1629.014*deep_arg->eosq+1083.435 * eoc;
			g520=-532.114+3017.977*eq-5740*deep_arg->eosq+3708.276*eoc;
		}

		else
		{
			g211=-72.099+331.819*eq-508.738*deep_arg->eosq+266.724*eoc;
			g310=-346.844+1582.851*eq-2415.925*deep_arg->eosq+1246.113*eoc;
			g322=-342.585+1554.908*eq-2366.899*deep_arg->eosq+1215.972*eoc;
			g410=-1052.797+4758.686*eq-7193.992*deep_arg->eosq+3651.957*eoc;
			g422=-3581.69+16178.11*eq-24462.77*deep_arg->eosq+12422.52*eoc;

			if (eq<=0.715)
				g520=1464.74-4664.75*eq+3763.64*deep_arg->eosq;

			else
				g520=-5149.66+29936.92*eq-54087.36*deep_arg->eosq+31324.56*eoc;
		}

		if (eq<0.7)
		{
			g533=-919.2277+4988.61*eq-9064.77*deep_arg->eosq+5542.21*eoc;
			g521=-822.71072+4568.6173*eq-8491.4146*deep_arg->eosq+5337.524*eoc;
			g532=-853.666+4690.25*eq-8624.77*deep_arg->eosq+5341.4*eoc;
		}

		else
		{
			g533=-37995.78+161616.52*eq-229838.2*deep_arg->eosq+109377.94*eoc;
			g521 =-51752.104+218913.95*eq-309468.16*deep_arg->eosq+146349.42*eoc;
			g532 =-40023.88+170470.89*eq-242699.48*deep_arg->eosq+115605.82*eoc;
		}

		sini2=deep_arg->sinio*deep_arg->sinio;
		f220=0.75*(1+2*deep_arg->cosio+deep_arg->theta2);
		f221=1.5*sini2;
		f321=1.875*deep_arg->sinio*(1-2*deep_arg->cosio-3*deep_arg->theta2);
		f322=-1.875*deep_arg->sinio*(1+2*deep_arg->cosio-3*deep_arg->theta2);
		f441=35*sini2*f220;
		f442=39.3750*sini2*sini2;
		f522=9.84375*deep_arg->sinio*(sini2*(1-2*deep_arg->cosio-5*deep_arg->theta2)+0.33333333*(-2+4*deep_arg->cosio+6*deep_arg->theta2));
		f523=deep_arg->sinio*(4.92187512*sini2*(-2-4*deep_arg->cosio+10*deep_arg->theta2)+6.56250012*(1+2*deep_arg->cosio-3*deep_arg->theta2));
		f542=29.53125*deep_arg->sinio*(2-8*deep_arg->cosio+deep_arg->theta2*(-12+8*deep_arg->cosio+10*deep_arg->theta2));
		f543=29.53125*deep_arg->sinio*(-2-8*deep_arg->cosio+deep_arg->theta2*(12+8*deep_arg->cosio-10*deep_arg->theta2));
		xno2=m->xnq*m->xnq;
		ainv2=aqnv*aqnv;
		temp1=3*xno2*ainv2;
		temp=temp1*ROOT22;
		m->d2201=temp*f220*g201;
		m->d2211=temp*f221*g211;
		temp1=temp1*aqnv;
		temp=temp1*ROOT32;
		m->d3210=temp*f321*g310;
		m->d3222=temp*f322*g322;
		temp1=temp1*aqnv;
		temp=2*temp1*ROOT44;
		m->d4410=temp*f441*g410;
		m->d4422=temp*f442*g422;
		temp1=temp1*aqnv;
		temp=temp1*ROOT52;
		m->d5220=temp*f522*g520;
		m->d5232=temp*f523*g532;
		temp=2*temp1*ROOT54;
		m->d5421=temp*f542*g521;
		m->d5433=temp*f543*g533;
		m->xlamo=xmao+m->xnodeo+m->xnodeo-m->thgr-m->thgr;
		bfact=deep_arg->xmdot+deep_arg->xnodot+deep_arg->xnodot-THDT-THDT;
		bfact=bfact+m->ssl+m->ssh+m->ssh;
	}

	else
	{
		m->resonanceFlag = 1;
		m->synchronousFlag = 1;

		/* Synchronous resonance terms initialization */
		g200=1+deep_arg->eosq*(-2.5+0.8125*deep_arg->eosq);
		g310=1+2*deep_arg->eosq;
		g300=1+deep_arg->eosq*(-6+6.60937*deep_arg->eosq);
		f220=0.75*(1+deep_arg->cosio)*(1+deep_arg->cosio);
		f311=0.9375*deep_arg->sinio*deep_arg->sinio*(1+3*deep_arg->cosio)-0.75*(1+deep_arg->cosio);
		f330=1+deep_arg->cosio;
		f330=1.875*f330*f330*f330;
		m->del1=3*m->xnq*m->xnq*aqnv*aqnv;
		m->del2=2*m->del1*f220*g200*Q22;
		m->del3=3*m->del1*f330*g300*Q33*aqnv;
		m->del1=m->del1*f311*g310*Q31*aqnv;
		m->fasx2=0.13130908;
		m->fasx4=2.8843198;
		m->fasx6=0.37448087;
		m->xlamo=xmao+m->xnodeo+m->omegao-m->thgr;
		bfact=deep_arg->xmdot+xpidot-THDT;
		bfact=bfact+m->ssl+m->ssg+m->ssh;
	}

	m->xfact=bfact-m->xnq;

	/* Initialize integrator */
	m->stepp=720;
	m->stepn=-720;
	m->step2=259200;

	return;
}

void deep_arg_dynamic_init(const struct _sdp4 *m, deep_arg_dynamic_t *deep_dyn){
	deep_dyn->savtsn=1E20;
	deep_dyn->loopFlag = 0;
	deep_dyn->epochRestartFlag = 0;
	deep_dyn->xli=m->xlamo;
	deep_dyn->xni=m->xnq;
	deep_dyn->atime=0;
}

void sdp4_deep(const struct _sdp4 *m, int ientry, const deep_arg_fixed_t * deep_arg, deep_arg_dynamic_t *deep_dyn)
{
	/* This function is used by SDP4 to add lunar and solar */
	/* perturbation effects to deep-space orbit objects.    */

	double alfdp,
	sinis, sinok, sil, betdp, dalf, cosis, cosok, dbet, dls, f2,
	f3, xnoh, pgh, ph, sel, ses, xls, sinzf, sis, sll, sls, temp,
	x2li, x2omi, xl, xldot, xnddt,
	xndot, xomi, zf, zm,
	delt=0, ft=0;


	switch (ientry)
	{

		case DPSecular:  /* Entrance for deep space secular effects */

		deep_dyn->xll=deep_dyn->xll+m->ssl*deep_dyn->t;
		deep_dyn->omgadf=deep_dyn->omgadf+m->ssg*deep_dyn->t;
		deep_dyn->xnode=deep_dyn->xnode+m->ssh*deep_dyn->t;
		deep_dyn->em=m->eo+m->sse*deep_dyn->t;
		deep_dyn->xinc=m->xincl+m->ssi*deep_dyn->t;

		if (deep_dyn->xinc<0)
		{
			deep_dyn->xinc=-deep_dyn->xinc;
			deep_dyn->xnode=deep_dyn->xnode+PI;
			deep_dyn->omgadf=deep_dyn->omgadf-PI;
		}

		if (!m->resonanceFlag) {
			return;
		}

		do
		{
			if ((deep_dyn->atime==0) || ((deep_dyn->t>=0) && (deep_dyn->atime<0)) || ((deep_dyn->t<0) && (deep_dyn->atime>=0)))
			{
				/* Epoch restart */

				if (deep_dyn->t>=0)
					delt=m->stepp;
				else
					delt=m->stepn;

				deep_dyn->atime=0;
				deep_dyn->xni=m->xnq;
				deep_dyn->xli=m->xlamo;
			}

			else
			{
				if (fabs(deep_dyn->t)>=fabs(deep_dyn->atime))
				{
					if (deep_dyn->t>0)
						delt=m->stepp;
					else
						delt=m->stepn;
				}
			}

			do
			{
				if (fabs(deep_dyn->t-deep_dyn->atime)>=m->stepp)
				{
					deep_dyn->loopFlag = 1;
					deep_dyn->epochRestartFlag = 0;
				}

				else
				{
					ft=deep_dyn->t-deep_dyn->atime;
					deep_dyn->loopFlag = 0;
				}

				if (fabs(deep_dyn->t)<fabs(deep_dyn->atime))
				{
					if (deep_dyn->t>=0)
						delt=m->stepn;
					else
						delt=m->stepp;

					deep_dyn->loopFlag = 1;
					deep_dyn->epochRestartFlag = 1;
				}

				/* Dot terms calculated */
				if (m->synchronousFlag) {
					xndot=m->del1*sin(deep_dyn->xli-m->fasx2)+m->del2*sin(2*(deep_dyn->xli-m->fasx4))+m->del3*sin(3*(deep_dyn->xli-m->fasx6));
					xnddt=m->del1*cos(deep_dyn->xli-m->fasx2)+2*m->del2*cos(2*(deep_dyn->xli-m->fasx4))+3*m->del3*cos(3*(deep_dyn->xli-m->fasx6));
				}

				else
				{
					xomi=m->omegaq+deep_arg->omgdot*deep_dyn->atime;
					x2omi=xomi+xomi;
					x2li=deep_dyn->xli+deep_dyn->xli;
					xndot=m->d2201*sin(x2omi+deep_dyn->xli-G22)+m->d2211*sin(deep_dyn->xli-G22)+m->d3210*sin(xomi+deep_dyn->xli-G32)+m->d3222*sin(-xomi+deep_dyn->xli-G32)+m->d4410*sin(x2omi+x2li-G44)+m->d4422*sin(x2li-G44)+m->d5220*sin(xomi+deep_dyn->xli-G52)+m->d5232*sin(-xomi+deep_dyn->xli-G52)+m->d5421*sin(xomi+x2li-G54)+m->d5433*sin(-xomi+x2li-G54);
					xnddt=m->d2201*cos(x2omi+deep_dyn->xli-G22)+m->d2211*cos(deep_dyn->xli-G22)+m->d3210*cos(xomi+deep_dyn->xli-G32)+m->d3222*cos(-xomi+deep_dyn->xli-G32)+m->d5220*cos(xomi+deep_dyn->xli-G52)+m->d5232*cos(-xomi+deep_dyn->xli-G52)+2*(m->d4410*cos(x2omi+x2li-G44)+m->d4422*cos(x2li-G44)+m->d5421*cos(xomi+x2li-G54)+m->d5433*cos(-xomi+x2li-G54));
				}

				xldot=deep_dyn->xni+m->xfact;
				xnddt=xnddt*xldot;

				if (deep_dyn->loopFlag) {
					deep_dyn->xli=deep_dyn->xli+xldot*delt+xndot*m->step2;
					deep_dyn->xni=deep_dyn->xni+xndot*delt+xnddt*m->step2;
					deep_dyn->atime=deep_dyn->atime+delt;
				}
			} while (deep_dyn->loopFlag && !deep_dyn->epochRestartFlag);
		} while (deep_dyn->loopFlag && deep_dyn->epochRestartFlag);

		deep_dyn->xn=deep_dyn->xni+xndot*ft+xnddt*ft*ft*0.5;
		xl=deep_dyn->xli+xldot*ft+xndot*ft*ft*0.5;
		temp=-deep_dyn->xnode+m->thgr+deep_dyn->t*THDT;

		if (!m->synchronousFlag) {
			deep_dyn->xll=xl+temp+temp;
		}else{
			deep_dyn->xll=xl-deep_dyn->omgadf+temp;
		}

		return;

		case DPPeriodic:	 /* Entrance for lunar-solar periodics */
		sinis=sin(deep_dyn->xinc);
		cosis=cos(deep_dyn->xinc);

		if (fabs(deep_dyn->savtsn-deep_dyn->t)>=30)
		{
			deep_dyn->savtsn=deep_dyn->t;
			zm=m->zmos+ZNS*deep_dyn->t;
			zf=zm+2*ZES*sin(zm);
			sinzf=sin(zf);
			f2=0.5*sinzf*sinzf-0.25;
			f3=-0.5*sinzf*cos(zf);
			ses=m->se2*f2+m->se3*f3;
			sis=m->si2*f2+m->si3*f3;
			sls=m->sl2*f2+m->sl3*f3+m->sl4*sinzf;
			deep_dyn->sghs=m->sgh2*f2+m->sgh3*f3+m->sgh4*sinzf;
			deep_dyn->shs=m->sh2*f2+m->sh3*f3;
			zm=m->zmol+ZNL*deep_dyn->t;
			zf=zm+2*ZEL*sin(zm);
			sinzf=sin(zf);
			f2=0.5*sinzf*sinzf-0.25;
			f3=-0.5*sinzf*cos(zf);
			sel=m->ee2*f2+m->e3*f3;
			sil=m->xi2*f2+m->xi3*f3;
			sll=m->xl2*f2+m->xl3*f3+m->xl4*sinzf;
			deep_dyn->sghl=m->xgh2*f2+m->xgh3*f3+m->xgh4*sinzf;
			deep_dyn->sh1=m->xh2*f2+m->xh3*f3;
			deep_dyn->pe=ses+sel;
			deep_dyn->pinc=sis+sil;
			deep_dyn->pl=sls+sll;
		}

		pgh=deep_dyn->sghs+deep_dyn->sghl;
		ph=deep_dyn->shs+deep_dyn->sh1;
		deep_dyn->xinc=deep_dyn->xinc+deep_dyn->pinc;
		deep_dyn->em=deep_dyn->em+deep_dyn->pe;

		if (m->xqncl>=0.2)
		{
			/* Apply periodics directly */
			ph=ph/deep_arg->sinio;
			pgh=pgh-deep_arg->cosio*ph;
			deep_dyn->omgadf=deep_dyn->omgadf+pgh;
			deep_dyn->xnode=deep_dyn->xnode+ph;
			deep_dyn->xll=deep_dyn->xll+deep_dyn->pl;
		}

		else
		{
			/* Apply periodics with Lyddane modification */
			sinok=sin(deep_dyn->xnode);
			cosok=cos(deep_dyn->xnode);
			alfdp=sinis*sinok;
			betdp=sinis*cosok;
			dalf=ph*cosok+deep_dyn->pinc*cosis*sinok;
			dbet=-ph*sinok+deep_dyn->pinc*cosis*cosok;
			alfdp=alfdp+dalf;
			betdp=betdp+dbet;
			deep_dyn->xnode=FMod2p(deep_dyn->xnode);
			xls=deep_dyn->xll+deep_dyn->omgadf+cosis*deep_dyn->xnode;
			dls=deep_dyn->pl+pgh-deep_dyn->pinc*deep_dyn->xnode*sinis;
			xls=xls+dls;
			xnoh=deep_dyn->xnode;
			deep_dyn->xnode=atan2(alfdp,betdp);

			/* This is a patch to Lyddane modification */
			/* suggested by Rob Matson. */

			if (fabs(xnoh-deep_dyn->xnode)>PI)
			{
			      if (deep_dyn->xnode<xnoh)
				  deep_dyn->xnode+=TWO_PI;
			      else
				  deep_dyn->xnode-=TWO_PI;
			}

			deep_dyn->xll=deep_dyn->xll+deep_dyn->pl;
			deep_dyn->omgadf=xls-deep_dyn->xll-cos(deep_dyn->xinc)*deep_dyn->xnode;
		}
		return;
	}
}

