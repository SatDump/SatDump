#include "sgp4.h"

#include "defs.h"
#include "unsorted.h"

void sgp4_init(const predict_orbital_elements_t *orbital_elements, struct _sgp4 *m)
{
	m->simpleFlag = 0;

	//Calculate old TLE field values as used in the original sgp4
	double temp_tle = TWO_PI/MINUTES_PER_DAY/MINUTES_PER_DAY;
	m->bstar = orbital_elements->bstar_drag_term / AE;
	m->xincl = orbital_elements->inclination * M_PI / 180.0;
	m->xnodeo = orbital_elements->right_ascension * M_PI / 180.0;
	m->eo = orbital_elements->eccentricity;
	m->omegao = orbital_elements->argument_of_perigee * M_PI / 180.0;
	m->xmo = orbital_elements->mean_anomaly * M_PI / 180.0;
	m->xno = orbital_elements->mean_motion*temp_tle*MINUTES_PER_DAY;

	double 	x1m5th, xhdot1,
	a1, a3ovk2, ao,
	betao, betao2, c1sq, c2, c3, coef, coef1, del1, delo, eeta, eosq,
	etasq, perigee, pinvsq, psisq, qoms24, s4, temp, temp1, temp2,
	temp3, theta2, theta4, tsi;

	/* Recover original mean motion (m->xnodp) and   */
	/* semimajor axis (m->aodp) from input elements. */

	a1=pow(XKE/m->xno,TWO_THIRD);
	m->cosio=cos(m->xincl);
	theta2=m->cosio*m->cosio;
	m->x3thm1=3*theta2-1.0;
	eosq=m->eo*m->eo;
	betao2=1.0-eosq;
	betao=sqrt(betao2);
	del1=1.5*CK2*m->x3thm1/(a1*a1*betao*betao2);
	ao=a1*(1.0-del1*(0.5*TWO_THIRD+del1*(1.0+134.0/81.0*del1)));
	delo=1.5*CK2*m->x3thm1/(ao*ao*betao*betao2);
	m->xnodp=m->xno/(1.0+delo);
	m->aodp=ao/(1.0-delo);

	/* For perigee less than 220 kilometers, the "simple"     */
	/* flag is set and the equations are truncated to linear  */
	/* variation in sqrt a and quadratic variation in mean    */
	/* anomaly.  Also, the c3 term, the delta omega term, and */
	/* the delta m term are dropped.                          */

	if ((m->aodp*(1-m->eo)/AE)<(220/EARTH_RADIUS_KM_WGS84+AE))
		m->simpleFlag = true;
	else
		m->simpleFlag = false;

	/* For perigees below 156 km, the      */
	/* values of s and qoms2t are altered. */

	s4=S_DENSITY_PARAM;
	qoms24=QOMS2T;
	perigee=(m->aodp*(1-m->eo)-AE)*EARTH_RADIUS_KM_WGS84;

	if (perigee<156.0)
	{
		if (perigee<=98.0)
		    s4=20;
		else
		 s4=perigee-78.0;

		qoms24=pow((120-s4)*AE/EARTH_RADIUS_KM_WGS84,4);
		s4=s4/EARTH_RADIUS_KM_WGS84+AE;
	}

	pinvsq=1/(m->aodp*m->aodp*betao2*betao2);
	tsi=1/(m->aodp-s4);
	m->eta=m->aodp*m->eo*tsi;
	etasq=m->eta*m->eta;
	eeta=m->eo*m->eta;
	psisq=fabs(1-etasq);
	coef=qoms24*pow(tsi,4);
	coef1=coef/pow(psisq,3.5);
	c2=coef1*m->xnodp*(m->aodp*(1+1.5*etasq+eeta*(4+etasq))+0.75*CK2*tsi/psisq*m->x3thm1*(8+3*etasq*(8+etasq)));
	m->c1=m->bstar*c2;
	m->sinio=sin(m->xincl);
	a3ovk2=-J3_HARMONIC_WGS72/CK2*pow(AE,3);
	c3=coef*tsi*a3ovk2*m->xnodp*AE*m->sinio/m->eo;
	m->x1mth2=1-theta2;

	m->c4=2*m->xnodp*coef1*m->aodp*betao2*(m->eta*(2+0.5*etasq)+m->eo*(0.5+2*etasq)-2*CK2*tsi/(m->aodp*psisq)*(-3*m->x3thm1*(1-2*eeta+etasq*(1.5-0.5*eeta))+0.75*m->x1mth2*(2*etasq-eeta*(1+etasq))*cos(2*m->omegao)));
	m->c5=2*coef1*m->aodp*betao2*(1+2.75*(etasq+eeta)+eeta*etasq);

	theta4=theta2*theta2;
	temp1=3*CK2*pinvsq*m->xnodp;
	temp2=temp1*CK2*pinvsq;
	temp3=1.25*CK4*pinvsq*pinvsq*m->xnodp;
	m->xmdot=m->xnodp+0.5*temp1*betao*m->x3thm1+0.0625*temp2*betao*(13-78*theta2+137*theta4);
	x1m5th=1-5*theta2;
	m->omgdot=-0.5*temp1*x1m5th+0.0625*temp2*(7-114*theta2+395*theta4)+temp3*(3-36*theta2+49*theta4);
	xhdot1=-temp1*m->cosio;
	m->xnodot=xhdot1+(0.5*temp2*(4-19*theta2)+2*temp3*(3-7*theta2))*m->cosio;
	m->omgcof=m->bstar*c3*cos(m->omegao);
	m->xmcof=-TWO_THIRD*coef*m->bstar*AE/eeta;
	m->xnodcf=3.5*betao2*xhdot1*m->c1;
	m->t2cof=1.5*m->c1;
	m->xlcof=0.125*a3ovk2*m->sinio*(3+5*m->cosio)/(1+m->cosio);
	m->aycof=0.25*a3ovk2*m->sinio;
	m->delmo=pow(1+m->eta*cos(m->xmo),3);
	m->sinmo=sin(m->xmo);
	m->x7thm1=7*theta2-1;

	if (!m->simpleFlag) {
		c1sq=m->c1*m->c1;
		m->d2=4*m->aodp*tsi*c1sq;
		temp=m->d2*tsi*m->c1/3;
		m->d3=(17*m->aodp+s4)*temp;
		m->d4=0.5*temp*m->aodp*tsi*(221*m->aodp+31*s4)*m->c1;
		m->t3cof=m->d2+2*c1sq;
		m->t4cof=0.25*(3*m->d3+m->c1*(12*m->d2+10*c1sq));
		m->t5cof=0.2*(3*m->d4+12*m->c1*m->d3+6*m->d2*m->d2+15*c1sq*(2*m->d2+c1sq));
	}
}

void sgp4_predict(const struct _sgp4 *m, double tsince, struct model_output *output)
{
	double cosuk, sinuk, rfdotk, vx, vy, vz, ux, uy, uz, xmy, xmx, cosnok,
	sinnok, cosik, sinik, rdotk, xinck, xnodek, uk, rk, cos2u, sin2u,
	u, sinu, cosu, betal, rfdot, rdot, r, pl, elsq, esine, ecose, epw,
	cosepw, tfour, sinepw, capu, ayn, xlt, aynl, xll,
	axn, xn, beta, xl, e, a, tcube, delm, delomg, templ, tempe, tempa,
	xnode, tsq, xmp, omega, xnoddf, omgadf, xmdf, temp, temp1, temp2,
	temp3, temp4, temp5, temp6;

	int i;

	/* Update for secular gravity and atmospheric drag. */
	xmdf=m->xmo+m->xmdot*tsince;
	omgadf=m->omegao+m->omgdot*tsince;
	xnoddf=m->xnodeo+m->xnodot*tsince;
	omega=omgadf;
	xmp=xmdf;
	tsq=tsince*tsince;
	xnode=xnoddf+m->xnodcf*tsq;
	tempa=1-m->c1*tsince;
	tempe=m->bstar*m->c4*tsince;
	templ=m->t2cof*tsq;

	if (!m->simpleFlag) {

		delomg=m->omgcof*tsince;
		delm=m->xmcof*(pow(1+m->eta*cos(xmdf),3)-m->delmo);
		temp=delomg+delm;
		xmp=xmdf+temp;
		omega=omgadf-temp;
		tcube=tsq*tsince;
		tfour=tsince*tcube;
		tempa=tempa-m->d2*tsq-m->d3*tcube-m->d4*tfour;
		tempe=tempe+m->bstar*m->c5*(sin(xmp)-m->sinmo);
		templ=templ+m->t3cof*tcube+tfour*(m->t4cof+tsince*m->t5cof);
	}

	a=m->aodp*pow(tempa,2);
	e=m->eo-tempe;
	xl=xmp+omega+xnode+m->xnodp*templ;
	beta=sqrt(1-e*e);
	xn=XKE/pow(a,1.5);

	/* Long period periodics */
	axn=e*cos(omega);
	temp=1/(a*beta*beta);
	xll=temp*m->xlcof*axn;
	aynl=temp*m->aycof;
	xlt=xl+xll;
	ayn=e*sin(omega)+aynl;

	/* Solve Kepler's Equation */
	capu=FMod2p(xlt-xnode);
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

		if (fabs(epw-temp2)<= E6A)
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
	xnodek=xnode+1.5*temp2*m->cosio*sin2u;
	xinck=m->xincl+1.5*temp2*m->cosio*m->sinio*cos2u;
	rdotk=rdot-xn*temp1*m->x1mth2*sin2u;
	rfdotk=rfdot+xn*temp1*(m->x1mth2*cos2u+1.5*m->x3thm1);

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
	output->phase=xlt-xnode-omgadf+TWO_PI;

	if (output->phase<0.0)
		output->phase+=TWO_PI;

	output->phase=FMod2p(output->phase);

	output->xinck = xinck;
	output->omgadf = omgadf;
	output->xnodek = xnodek;

}
