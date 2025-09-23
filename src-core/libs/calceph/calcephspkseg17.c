/*-----------------------------------------------------------------*/
/*!
  \file calcephspkseg17.c
  \brief compute position and velocity from SPICE KERNEL ephemeris file
         for the segments of type 17.

  \author  M. Gastineau
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de
  Paris.

   Copyright, 2018-2024, CNRS
   email of the author : Mickael.Gastineau@obspm.fr

  History:
*/
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* License  of this file :
 This file is "triple-licensed", you have to choose one  of the three licenses
 below to apply on this file.

    CeCILL-C
        The CeCILL-C license is close to the GNU LGPL.
        ( http://www.cecill.info/licences/Licence_CeCILL-C_V1-en.html )

 or CeCILL-B
        The CeCILL-B license is close to the BSD.
        (http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.txt)

 or CeCILL v2.1
      The CeCILL license is compatible with the GNU GPL.
      ( http://www.cecill.info/licences/Licence_CeCILL_V2.1-en.html )

This library is governed by the CeCILL-C, CeCILL-B or the CeCILL license under
French law and abiding by the rules of distribution of free software.
You can  use, modify and/ or redistribute the software under the terms
of the CeCILL-C,CeCILL-B or CeCILL license as circulated by CEA, CNRS and INRIA
at the following URL "http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL-C,CeCILL-B or CeCILL license and that you accept its
terms.
*/
/*-----------------------------------------------------------------*/

#include "calcephconfig.h"
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#include <float.h>
#if HAVE_MATH_H
#include <math.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "calcephdebug.h"
#include "real.h"
#define __CALCEPH_WITHIN_CALCEPH 1
#include "calceph.h"
#include "calcephinternal.h"
#include "calcephspice.h"
#include "util.h"

/*--------------------------------------------------------------------------*/
/*!
     This function solves the kepler equation  using a newton method:

     ml = F + h *cos(F) - k*sin(F)

     It returns the value F.

  @param ml (in) mean longitude
  @param h (in) h term
  @param k (in) k term
*/
/*--------------------------------------------------------------------------*/
double calceph_solve_kepler(double ml, double h, double k)
{
    double F = ml;

    int j;

    for (j = 0; j < 15; j++)
    {
        double ca = cos(F);
        double sa = sin(F);
        double se = k * sa - h * ca;
        double fa = F - se - ml;
        double ce = k * ca + h * sa;
        double f1a = 1.E0 - ce;
        double d1 = -fa / f1a;

        F = F + d1;
        if (fabs(d1) < 1.2E0 * DBL_EPSILON * fabs(F))
            break;
    }
    return F;
}

/*--------------------------------------------------------------------------*/
/*!
     This function read the data block corresponding to the specified Time
     if this block isn't already in memory.

     This function works only over the specified segment.
     The segment must be of type 17.

     It computes position and velocity vectors for a selected
     planetary body from Chebyshev coefficients.

    The segment 17 contains equinoctial elements.  The equinoctial elements are
  given by:
  a = a , h = e sin(ω + IΩ), k = e cos(ω + IΩ), p =tan(i/2)sinΩ, q
  =tan(i/2)cosΩ, λ = M + ω + IΩ

     Reference : https://apps.dtic.mil/dtic/tr/fulltext/u2/a531136.pdf

     SEMIANALYTIC SATELLITE THEORY
     D. A. Danielson,C. P. Sagovac, B. Neta, L. W. Early
     Mathematics Department
     Naval Postgraduate School
     Monterey, CA 93943

  @param pspk (inout) SPK file
  @param seg (in) segment where is located TimeJD0+timediff
  @param cache (inout) cache for the object
  @param TimeJD0  (in) Time for which position is desired (Julian Date)
  @param Timediff  (in) offset time from TimeJD0 for which position is desired
  (Julian Date)
  @param Planet (out) position and velocity
         Planet has position in km
         Planet has velocity in km/sec
*/
/*--------------------------------------------------------------------------*/
int calceph_spk_interpol_PV_segment_17(struct SPKfile *PARAMETER_UNUSED(pspk),
                                       struct SPKSegmentHeader *seg,
                                       struct SPICEcache *PARAMETER_UNUSED(cache), treal TimeJD0,
                                       treal Timediff, stateType * Planet)
{
#if HAVE_PRAGMA_UNUSED
#pragma unused(pspk, cache)
#endif
    const double Timesec = ((TimeJD0 - 2.451545E+06) + Timediff) * 86400E0;

    double epoch = seg->seginfo.data17.epoch;
    double a = seg->seginfo.data17.a;
    double h0 = seg->seginfo.data17.h;
    double k0 = seg->seginfo.data17.k;
    double p0 = seg->seginfo.data17.p;
    double q0 = seg->seginfo.data17.q;
    double ml0 = seg->seginfo.data17.mean_longitude;
    double ra = seg->seginfo.data17.ra_pole;
    double de = seg->seginfo.data17.de_pole;
    double dlpdt = seg->seginfo.data17.rate_longitude_periapse;
    double dnodedt = seg->seginfo.data17.longitude_ascending_node_rate;
    double dmldt = seg->seginfo.data17.mean_longitude_rate;

    double dt;                  /* time from epoch */

    double h, k;                /* h an k at the time of the evaluation */

    double p, q;                /* p an q at the time of the evaluation */

    double vf[3], vg[3];        /* equinoctial reference frame basis vectors  : f and g  */

    double dI;

    double ml;                  /* mean logntiude a t the evaluation */

    double twopi = 2.E0 * atan2(0.E0, -1.E0);   /* 2*PI */

    double b;

    double transeqtoinertial[3][3]; /* matrix transformation from equatorial
                                       plane to inertial plane */

    double X, Y, XP, YP, r;     /* coordinates in orbital plane */

    double DX, DY;

    double Xeq[3], XPeq[3];     /* coordinates in the equinoctal reference frame */

    double nfac, prate, vpnode[3];

    double F;                   /* excentric longitude */

    int j;

    /* check the order */
    if (Planet->order >= 2)
    {
        fatalerror("order>=2 is not supported on segment of type 17");
        return 0;
    }

    /* compute k, h, q and p at the time  of the evaluation */
    dt = Timesec - epoch;
    h = h0 * cos(dlpdt * dt) + k0 * sin(dlpdt * dt);
    k = -h0 * sin(dlpdt * dt) + k0 * cos(dlpdt * dt);

    p = p0 * cos(dnodedt * dt) + q0 * sin(dnodedt * dt);
    q = -p0 * sin(dnodedt * dt) + q0 * cos(dnodedt * dt);

    /* compute the vector f and g (eq. 1 of 2.1.4 - page 7) */
    dI = 1.0E0 / (1.0E0 + p * p + q * q);

    vf[0] = (1.0E0 - p * p + q * q) * dI;
    vf[1] = (2.0E0 * p * q) * dI;
    vf[2] = (-2.0E0 * p) * dI;

    vg[0] = (2.0E0 * p * q) * dI;
    vg[1] = (1.0E0 + p * p - q * q) * dI;
    vg[2] = (2.0E0 * q) * dI;

    /* compute the mean longitude and excentric longitude */
    ml = fmod(ml0 + dmldt * dt, twopi);
    F = calceph_solve_kepler(ml, h, k);

    /* compute the positions and velocities X and Y in the orbital plane : (eq. 4,
     * 7, 8 of 2.1.4 - page 8) */
    b = 1.0E0 / (1.0E0 + sqrt(1.E0 - h * h - k * k));
    X = a * ((1.E0 - h * h * b) * cos(F) + h * k * b * sin(F) - k);
    Y = a * ((1.E0 - k * k * b) * sin(F) + h * k * b * cos(F) - h);
    r = a * (1.E0 - h * sin(F) - k * cos(F));
    XP = (dmldt * a * a / r) * (h * k * b * cos(F) - (1.E0 - h * h * b) * sin(F));
    YP = (dmldt * a * a / r) * ((1.E0 - k * k * b) * cos(F) - h * k * b * sin(F));

    /* compute the positions and velocities in the equinoctal reference frame  :
       (eq. 9 of 2.1.4 - page 8)
       modify the velocity
     */
    nfac = 1.0E0 - (dlpdt / dmldt);
    prate = dlpdt - dnodedt;
    DX = nfac * XP - prate * Y;
    DY = nfac * YP + prate * X;

    for (j = 0; j < 3; j++)
    {
        Xeq[j] = X * vf[j] + Y * vg[j];
        XPeq[j] = DX * vf[j] + DY * vg[j];
    }

    vpnode[0] = -dnodedt * Xeq[1];
    vpnode[1] = dnodedt * Xeq[0];
    vpnode[2] = 0.E0;
    for (j = 0; j < 3; j++)
    {
        XPeq[j] += vpnode[j];
    }
    for (j = 0; j < 3; j++)
    {
        Planet->Position[j] = Xeq[j];
        Planet->Velocity[j] = XPeq[j];
    }

    /* compute the positions and the velocities in the inertial refrence frame */
    transeqtoinertial[0][0] = -sin(ra);
    transeqtoinertial[0][1] = -cos(ra) * sin(de);
    transeqtoinertial[0][2] = cos(ra) * cos(de);
    transeqtoinertial[1][0] = cos(ra);
    transeqtoinertial[1][1] = -sin(ra) * sin(de);
    transeqtoinertial[1][2] = sin(ra) * cos(de);
    transeqtoinertial[2][0] = 0.;
    transeqtoinertial[2][1] = cos(de);
    transeqtoinertial[2][2] = sin(de);
    calceph_stateType_rotate_PV(Planet, transeqtoinertial);

    return 1;
}
