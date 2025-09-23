/*-----------------------------------------------------------------*/
/*!
  \file calcephspkseg5.c
  \brief perform I/O on SPICE KERNEL ephemeris data file
         for the segments of type 5
         interpolate SPICE KERNEL Ephemeris data.
         for the segments of type 5
         compute position and velocity from SPICE KERNEL ephemeris file
         for the segments of type 5.

  \author  M. Gastineau
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de Paris.

   Copyright, 2019, CNRS
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
knowledge of the CeCILL-C,CeCILL-B or CeCILL license and that you accept its terms.
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
/* enable M_PI with windows sdk */
#define _USE_MATH_DEFINES
#if HAVE_MATH_H
#include <math.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "calcephdebug.h"
#include "real.h"
#define __CALCEPH_WITHIN_CALCEPH 1
#include "calceph.h"
#include "calcephspice.h"
#include "util.h"
#include "calcephinternal.h"

/*--------------------------------------------------------------------------*/
/* private functions */
/*--------------------------------------------------------------------------*/
static void calceph_spk_interpol_DiscreteState(double t1, stateType PV1, double t2, stateType PV2,
                                               double GM, double TimeJD0, double Timediff, stateType * Planet);
static void calceph_propagateTwoBody(const stateType * PV, double dt, double GM, stateType * PVdt);

/*--------------------------------------------------------------------------*/
/*  Two body propagtor : compute the position/veclocity
 at the time dt using the GM and the intial position PV.

@details

Based on
Ana Paula Marins Chiaradia, Hélio Koiti Kuga, and Antonio Fernando Bertachini de Almeida Prado,
“Comparison between Two Methods to Calculate the Transition Matrix of Orbit Motion,”
Mathematical Problems in Engineering, vol. 2012, Article ID 768973, 12 pages, 2012.

https://doi.org/10.1155/2012/768973

or

https://www.hindawi.com/journals/mpe/2012/768973/

@param PV (in) initial position/velocity
@param dt (in) delta time
@param GM (in) GM of the central body
@param PVdt (out) position/velocity at the time dt
*/
/*--------------------------------------------------------------------------*/
static void calceph_propagateTwoBody(const stateType * PV, double dt, double GM, stateType * PVdt)
{
    double r0, h0, v0, alpha, oneOverA, e, E0, eSinE0, eCosE0, E, dE;
    double n, M0, M, r, f, g, fp, gp, s0, s1, s2;
    int i;

    /* equation 3.2 */
    r0 = sqrt(PV->Position[0] * PV->Position[0] + PV->Position[1] * PV->Position[1]
              + PV->Position[2] * PV->Position[2]);
    h0 = PV->Position[0] * PV->Velocity[0] + PV->Position[1] * PV->Velocity[1] + PV->Position[2] * PV->Velocity[2];
    v0 = sqrt(PV->Velocity[0] * PV->Velocity[0] + PV->Velocity[1] * PV->Velocity[1]
              + PV->Velocity[2] * PV->Velocity[2]);
    alpha = v0 * v0 - 2. * GM / r0;
    oneOverA = -alpha / GM;

    /* equation 3.3 */
    eSinE0 = h0 / sqrt(GM / oneOverA);
    eCosE0 = 1.E0 - r0 * oneOverA;
    e = sqrt(eSinE0 * eSinE0 + eCosE0 * eCosE0);
    E0 = atan2(eSinE0, eCosE0);
    /* equation 3.4 */
    M0 = E0 - eSinE0;
    n = sqrt(GM * oneOverA * oneOverA * oneOverA);
    M = n * dt + M0;

    /* equation 3.5 */
    E = calceph_solve_kepler(M, 0.E0, e);
    dE = E - E0;

    /* equation 3.6 */
    s0 = cos(dE);
    s1 = sqrt(1.E0 / (GM * oneOverA)) * sin(dE);
    s2 = 1.E0 / (GM * oneOverA) * (1.E0 - s0);
    /* equation 3.7 -> 3.11 */
    r = r0 * s0 + h0 * s1 + GM * s2;
    f = 1.E0 - GM * s2 / r0;
    g = r0 * s1 + h0 * s2;
    fp = -GM * s1 / (r * r0);
    gp = 1.E0 - GM * s2 / (r);

    /* equation 3.12 */
    for (i = 0; i < 3; i++)
    {
        PVdt->Position[i] = PV->Position[i] * f + PV->Velocity[i] * g;
        PVdt->Velocity[i] = PV->Position[i] * fp + PV->Velocity[i] * gp;
    }
}

/*--------------------------------------------------------------------------*/
/*!  This function computes position and velocity vectors
    from the Discrete states (two body propagation).

 @param t1 (in) time of PV1
 @param PV1 (in) positon/velocity of the body a the time t1
 @param t2 (in) time of PV2
 @param PV2 (in) positon/velocity of the body a the time t2
 @param GM (in) GM of the central body
 @param TimeJD0  (in) Time in seconds from J2000 for which position is desired (Julian Date)
 @param Timediff  (in) offset time from TimeJD0 for which position is desired (Julian Date)
 @param Planet (out) Pointer to external array to receive the position/velocity.
   Planet has position in ephemerides units
   Planet has velocity in ephemerides units
*/
/*--------------------------------------------------------------------------*/
static void calceph_spk_interpol_DiscreteState(double t1, stateType PV1, double t2, stateType PV2,
                                               double GM, double TimeJD0, double Timediff, stateType * Planet)
{
    int i;
    double delta1 = (TimeJD0 + Timediff - t1);  /* = t-t1 */
    double delta2 = (TimeJD0 + Timediff - t2);  /* = t-t2 */
    double W;                   /* weighting function = W(t) = 0.5 + 0.5 * cos [ PI * ( t - t1 ) / ( t2 - t1 ) ] */
    double Wp;                  /* derivate of the weighting function =- 0.5 * PI *sin [ PI * ( t - t1 ) / ( t2 - t1 ) ]
                                   /(t2 -t1) */
    stateType PV1dt, PV2dt;

    calceph_propagateTwoBody(&PV1, delta1, GM, &PV1dt);
    calceph_propagateTwoBody(&PV2, delta2, GM, &PV2dt);

    W = 0.5E0 + 0.5E0 * cos(M_PI * delta1 / (t2 - t1));

    Wp = -0.5E0 * M_PI * sin(M_PI * delta1 / (t2 - t1)) / (t2 - t1);

    /* P = W(t) * P1(t)+ (1 - W(t)) * P2(t) */
    /* Compute position */
    for (i = 0; i < 3; i++)
    {
        Planet->Position[i] = W * PV1dt.Position[i] + (1.E0 - W) * PV2dt.Position[i];
    }

    /* V = W(t) * V1(t) + (1 - W(t)) * V2(t) + W'(t) * ( P1(t) - P2(t) ) */
    if (Planet->order >= 1)
    {
        /* Compute velocity */
        for (i = 0; i < 3; i++)
        {
            Planet->Velocity[i] = W * PV1dt.Velocity[i] + (1.E0 - W) * PV2dt.Velocity[i]
                + Wp * (PV1dt.Position[i] - PV2dt.Position[i]);
        }
    }
}

/*--------------------------------------------------------------------------*/
/*!
     This function read the data block corresponding to the specified Time
     if this block isn't already in memory.

     This function works only over the specified segment.
     The segment must be of type 5.

     It computes position and velocity vectors for a selected
     planetary body from the discrete state.

  @param pspk (inout) SPK file
    pspk is not updated if the data are already prefetched.
  @param seg (in) segment where is located TimeJD0+timediff
  @param cache (inout) cache for the object
  @param TimeJD0  (in) Time for which position is desired (Julian Date)
  @param Timediff  (in) offset time from TimeJD0 for which position is desired (Julian Date)
  @param Planet (out) position and velocity
         Planet has position in km
         Planet has velocity in km/sec
*/
/*--------------------------------------------------------------------------*/
int calceph_spk_interpol_PV_segment_5(struct SPKfile *pspk, struct SPKSegmentHeader *seg,
                                      struct SPICEcache *cache, treal TimeJD0, treal Timediff, stateType * Planet)
{
    int k;

    const double Timesec = ((TimeJD0 - 2.451545E+06) + Timediff) * 86400E0;

    int recT;                   /* record where is located Timesec */

    const double *drecord;
    const double *drecord6;

    int nrecord;

    int subdir = 0;

    int wordbegin, wordend;

    stateType PV1, PV2;
    double t1, t2;

    /* read all the epoch */
    int nreadepoch = seg->seginfo.data5.count_record;

    if (nreadepoch >= 100)
    {
        /* locate the correct nepoch slot using the fast directory */
        while (subdir < seg->seginfo.data5.count_directory && seg->seginfo.data5.directory[subdir] < Timesec)
        {
            subdir++;
        }
        subdir *= 100;
        /* load the subdirectory */
        wordbegin = seg->rec_begin + 6 * nreadepoch;
        wordend = wordbegin + nreadepoch - 1;

        if (calceph_spk_fastreadword(pspk, seg, cache, "", wordbegin, wordend, &drecord) == 0)
        {
            return 0;
        }
        drecord += subdir;
        nrecord = 100;
        if (subdir + nrecord >= nreadepoch)
            nrecord = nreadepoch - subdir;
    }
    else
    {                           /* less than 100 epochs */
        drecord = seg->seginfo.data5.directory;
        nrecord = nreadepoch;
    }

    /*find the record */
    recT = 0;
    while (recT < nrecord - 1 && drecord[recT] < Timesec)
        recT++;
    if (recT + subdir > 0)
    {
        recT--;
        t1 = drecord[recT];
        t2 = drecord[recT + 1];
    }
    else
    {
        t1 = drecord[recT];
        t2 = drecord[recT + 1];
    }
    recT += subdir;

    /* load the record recT */
    if (calceph_spk_fastreadword(pspk, seg, cache, "", seg->rec_begin + 6 * recT,
                                 seg->rec_begin + 6 * (recT + 1) + 5, &drecord6) == 0)
    {
        return 0;
    }

    /* fill the data structure with the record */
    for (k = 0; k < 3; k++)
    {
        PV1.Position[k] = drecord6[k];
        PV1.Velocity[k] = drecord6[k + 3];
        PV2.Position[k] = drecord6[k + 6];
        PV2.Velocity[k] = drecord6[k + 9];
    }

    /* check the order */
    if (Planet->order >= 2)
    {
        fatalerror("order>=2 is not supported on segment of type 5");
        return 0;
    }

    calceph_spk_interpol_DiscreteState(t1, PV1, t2, PV2, seg->seginfo.data5.GM,
                                       (TimeJD0 - 2.451545E+06) * 86400E0, Timediff * 86400E0, Planet);

#if DEBUG
    /* print the record */
    printf("Timesec=%23.16E\n", Timesec);
    printf("seg->seginfo.data5.count_record : %d\n", seg->seginfo.data5.count_record);
    printf("subdir=%d\n", subdir);
    printf("read : %d %d\n", wordbegin, wordend);
    if (recT >= seg->seginfo.data5.count_record)
        fatalerror("calceph_spk_interpol_PV_segment_5 cas 2 : %d %d\n", recT, nrecord);
    if (subdir >= seg->seginfo.data5.count_record)
        fatalerror("calceph_spk_interpol_PV_segment_5 cas 1 : %d %d\n", subdir, nrecord);
    printf("t1=%23.16E t2=%23.16E\n", t1, t2);

    calceph_stateType_debug(&PV1);
    calceph_stateType_debug(&PV2);
    calceph_stateType_debug(Planet);
#endif

    return 1;
}
