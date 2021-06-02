#ifndef _SDP4_H_
#define _SDP4_H_

#include "predict.h"

struct model_output {
	double xinck; //inclination?
	double omgadf; //argument of perigee?
	double xnodek; //RAAN?

	double pos[3];
	double vel[3];

	double phase;
};


/**
 * Parameters for deep space perturbations
 **/
typedef struct	{
	/* Used by dpinit part of Deep() */
	double  eosq, sinio, cosio, betao, aodp, theta2,
	sing, cosg, betao2, xmdot, omgdot, xnodot, xnodp;

	/* Used by thetg and Deep() */
	double  ds50;
}  deep_arg_fixed_t;

/**
 * Output from deep space perturbations.
 **/
typedef struct {
	/* Moved from deep_arg_t. */
	/* Used by dpsec and dpper parts of Deep() */
	double  xll, omgadf, xnode, em, xinc, xn, t;

	/* Previously a part of _sdp4, moved here. */
	double pl, pinc, pe, sh1, sghl, shs, savtsn, atime, xni, xli, sghs;
	///Do loop flag:
	int loopFlag;
	///Epoch restart flag:
	int epochRestartFlag;
} deep_arg_dynamic_t;


/**
 * Parameters relevant for SDP4 (simplified deep space perturbations) orbital model.
 **/
struct _sdp4 {

	///Lunar terms done?
	int lunarTermsDone;
	///Resonance flag:
	int resonanceFlag;
	///Synchronous flag:
	int synchronousFlag;


	///Static variables from SDP4():
	double x3thm1, c1, x1mth2, c4, xnodcf, t2cof, xlcof,
	aycof, x7thm1;
	deep_arg_fixed_t deep_arg;

	///Static variables from Deep():
	double thgr, xnq, xqncl, omegaq, zmol, zmos, ee2, e3,
	xi2, xl2, xl3, xl4, xgh2, xgh3, xgh4, xh2, xh3, sse, ssi, ssg, xi3,
	se2, si2, sl2, sgh2, sh2, se3, si3, sl3, sgh3, sh3, sl4, sgh4, ssl,
	ssh, d3210, d3222, d4410, d4422, d5220, d5232, d5421, d5433, del1,
	del2, del3, fasx2, fasx4, fasx6, xlamo, xfact, stepp,
	stepn, step2, preep, d2201, d2211,
	zsingl, zcosgl, zsinhl, zcoshl, zsinil, zcosil;

	//converted fields from predict_orbital_elements_t.
	double xnodeo;
	double omegao;
	double xmo;
	double xincl;
	double eo;
	double xno;
	double bstar;
	double epoch;
};

/**
 * Initialize SDP4 model parameters.
 *
 * \param orbital_elements Orbital elements
 * \param m Struct to initialize
 **/
void sdp4_init(const predict_orbital_elements_t *orbital_elements, struct _sdp4 *m);

/**
 * Predict ECI position and velocity of deep-space orbit (period > 225 minutes) according to SDP4 model and the given orbital parameters.
 *
 * \param m SDP4 model parameters
 * \param tsince Time since epoch of TLE in minutes
 * \param output Modeled output parameters
 * \copyright GPLv2+
 **/
void sdp4_predict(const struct _sdp4 *m, double tsince, struct model_output *output);

/**
 * Deep space perturbations. Original Deep() function.
 *
 * \param m SDP4 model parameters
 * \param ientry Behavior flag. 1: Deep space secular effects. 2: lunar-solar periodics
 * \param deep_arg Fixed deep perturbation parameters
 * \param deep_dyn Output of deep space perturbations
 * \copyright GPLv2+
 **/
void sdp4_deep(const struct _sdp4 *m, int ientry, const deep_arg_fixed_t * deep_arg, deep_arg_dynamic_t *deep_dyn);


#endif // ifndef _SDP4_H_
