#ifndef SGP4_H_
#define SGP4_H_

#include "predict.h"
#include "sdp4.h"

/**
 * Parameters relevant for SGP4 (simplified general perturbations) orbital model.
 **/
struct _sgp4 {
	
	///Simple flag
	int simpleFlag;

	///Static variables from original SGP4() (time-independent, and might probably have physical meaningfulness)
	double aodp, aycof, c1, c4, c5, cosio, d2, d3, d4, delmo,
	omgcof, eta, omgdot, sinio, xnodp, sinmo, t2cof, t3cof, t4cof,
	t5cof, x1mth2, x3thm1, x7thm1, xmcof, xmdot, xnodcf, xnodot, xlcof;

	//tle fields copied (and converted) from predict_orbital_t. The fields above are TLE-dependent anyway, and interrelated with the values below.
	double bstar;
	double xincl;
	double xnodeo;
	double eo;
	double omegao;
	double xmo;
	double xno;
};

/**
 * Initialize SGP4 model parameters.  
 *
 * \param orbital_elements Orbital elements
 * \param m Struct to initialize
 * \copyright GPLv2+
 **/
void sgp4_init(const predict_orbital_elements_t *orbital_elements, struct _sgp4 *m);

/**
 * Predict ECI position and velocity of near-earth orbit (period < 225 minutes) according to SGP4 model and the given orbital parameters. 
 *
 * \param m SGP4 model parameters
 * \param tsince Time since epoch of TLE in minutes
 * \param output Output of model
 * \copyright GPLv2+
 **/
void sgp4_predict(const struct _sgp4 *m, double tsince, struct model_output *output);

#endif
