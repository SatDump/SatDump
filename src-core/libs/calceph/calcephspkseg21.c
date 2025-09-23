/*-----------------------------------------------------------------*/
/*!
  \file calcephspkseg21.c
  \brief interpolate SPICE KERNEL Ephemeris data.
         for the segments of type 21
         compute position and velocity from SPICE KERNEL ephemeris file
         for the segments of type 21.

  \author  M. Gastineau
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de
  Paris.

   Copyright, 2018-2021, CNRS
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
#include "calcephinternal.h"
#include "calcephspice.h"
#include "util.h"

/*--------------------------------------------------------------------------*/
/* private types */
/*--------------------------------------------------------------------------*/
#define MAXTRM 50

/*! structure of the record for the segment type 1 or 21 */
struct SegType21_Record
{
    double tl;                  /*!< final epoch of record */
    double g[MAXTRM];           /*!< stepsize function vector */
    double refpos[3];           /*!< reference position vector */
    double refvel[3];           /*!< reference velocity vector */
    double dt[MAXTRM][3];       /*!< Modified divided difference arrays */
    int kqmax1;                 /*!< maximum integration order plus 1 */
    int kq[3];                  /*!< integration order array */
};

/*--------------------------------------------------------------------------*/
/* private functions */
/*--------------------------------------------------------------------------*/
static void calceph_spk_interpol_MDA_21(const struct SegType21_Record
                                        *Precord, double TimeJD0, double Timediff, stateType * Planet);

/*--------------------------------------------------------------------------*/
/*!  This function computes position and velocity vectors
     from the Extended Modified Difference Array.

 @param Precord (in) record of type 21
 @param TimeJD0  (in) Time in seconds from J2000 for which position is desired
 (Julian Date)
 @param Timediff  (in) offset time from TimeJD0 for which position is desired
 (Julian Date)
 @param Planet (out) Pointer to external array to receive the position/velocity.
   Planet has position in ephemerides units
   Planet has velocity in ephemerides units
*/
/*--------------------------------------------------------------------------*/
static void calceph_spk_interpol_MDA_21(const struct SegType21_Record
                                        *Precord, double TimeJD0, double Timediff, stateType * Planet)
{
    double delta = (TimeJD0 - Precord->tl) + Timediff;

    double tp = delta;

    int mpq2 = Precord->kqmax1 - 2;

    int ks = Precord->kqmax1 - 1;

    double fc[MAXTRM];

    double wc[MAXTRM - 1];

    double w[MAXTRM + 3];

    int i, j, jx;

    fc[0] = 1.E0;
    for (j = 0; j < mpq2; j++)
    {
        fc[j + 1] = tp / Precord->g[j];
        wc[j] = delta / Precord->g[j];
        tp = delta + Precord->g[j];
    }

    /* inverse of 1/(j+1) */
    for (j = 0; j <= Precord->kqmax1; j++)
    {
        w[j] = 1.E0 / ((double) (j + 1));
    }

    jx = 0;
    for (; ks >= 2; ks--)
    {
        int ks1 = ks - 1;

        jx++;
        for (j = 0; j < jx; j++)
        {
            w[j + ks] = fc[j + 1] * w[j + ks1] - wc[j] * w[j + ks];
        }
    }

    /* Compute position */
    for (i = 0; i < 3; i++)
    {
        double P_Sum = 0.E0;

        for (j = Precord->kq[i] - 1; j >= 0; j--)
        {
            P_Sum += Precord->dt[j][i] * w[j + ks];
        }
        Planet->Position[i] = Precord->refpos[i] + delta * (Precord->refvel[i] + delta * P_Sum);
    }

    /* Update w for velocity */
    if (Planet->order >= 1)
    {
        int ks1 = ks - 1;

        for (j = 0; j < jx; j++)
        {
            w[j + ks] = fc[j + 1] * w[j + ks1] - wc[j] * w[j + ks];
        }
        ks--;

        /* Compute velocity */
        for (i = 0; i < 3; i++)
        {
            double V_Sum = 0.E0;

            for (j = Precord->kq[i] - 1; j >= 0; j--)
            {
                V_Sum += Precord->dt[j][i] * w[j + ks];
            }
            Planet->Velocity[i] = Precord->refvel[i] + delta * V_Sum;
        }
    }
}

/*--------------------------------------------------------------------------*/
/*!
     This function read the data block corresponding to the specified Time
     if this block isn't already in memory.

     This function works only over the specified segment.
     The segment must be of type 21.

     It computes position and velocity vectors for a selected
     planetary body from Chebyshev coefficients.

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
int calceph_spk_interpol_PV_segment_21(struct SPKfile *pspk,
                                       struct SPKSegmentHeader *seg,
                                       struct SPICEcache *cache, treal TimeJD0, treal Timediff, stateType * Planet)
{
    int p, k;

    const double Timesec = ((TimeJD0 - 2.451545E+06) + Timediff) * 86400E0;

    int recT;                   /* record where is located Timesec */

    const double *drecord;

    int nrecord;

    int subdir = 0;

    int wordbegin, wordend;

    struct SegType21_Record arecord;

    int nvalid = seg->seginfo.data21.dlsize;    /* for segment type 1 = 15 */
    int dlsize = 4 * seg->seginfo.data21.dlsize + 11;

    /* read all the epoch */
    int nreadepoch = seg->seginfo.data21.count_record;

    if (nreadepoch >= 100)
    {
        /* locate the correct nepoch slot using the fast directory */
        while (subdir < seg->seginfo.data21.count_directory && seg->seginfo.data21.directory[subdir] < Timesec)
        {
            subdir++;
        }
        subdir *= 100;
        /* load the subdirectory */
        wordbegin = seg->rec_begin + dlsize * nreadepoch;
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
        drecord = seg->seginfo.data21.directory;
        nrecord = nreadepoch;
    }

    /*find the record */
    recT = 0;
    while (recT < nrecord - 1 && drecord[recT] < Timesec)
        recT++;
    recT += subdir;

    /* load the record */
    if (calceph_spk_fastreadword
        (pspk, seg, cache, "", seg->rec_begin + dlsize * recT, seg->rec_begin + dlsize * (recT + 1) - 1, &drecord) == 0)
    {
        return 0;
    }

    /* fill the data structure with the record */
    arecord.tl = drecord[0];
    for (k = 0; k < nvalid; k++)
        arecord.g[k] = drecord[1 + k];
    for (k = 0; k < 3; k++)
        arecord.refpos[k] = drecord[nvalid + 1 + 2 * k];
    for (k = 0; k < 3; k++)
        arecord.refvel[k] = drecord[nvalid + 2 + 2 * k];
    for (p = 0; p < nvalid; p++)
    {
        for (k = 0; k < 3; k++)
            arecord.dt[p][k] = drecord[nvalid + 7 + k * nvalid + p];
    }
    arecord.kqmax1 = (int) drecord[4 * nvalid + 7];
    for (k = 0; k < 3; k++)
        arecord.kq[k] = (int) drecord[4 * nvalid + 8 + k];

    /* check the order */
    if (Planet->order >= 2)
    {
        fatalerror("order>=2 is not supported on segment of type 21");
        return 0;
    }

    calceph_spk_interpol_MDA_21(&arecord, (TimeJD0 - 2.451545E+06) * 86400E0, Timediff * 86400E0, Planet);

#if DEBUG
    /* print the record */
    printf("Timesec=%23.16E\n", Timesec);
    printf("seg->seginfo.data21.count_record : %d\n", seg->seginfo.data21.count_record);
    printf("subdir=%d\n", subdir);
    printf("read : %d %d\n", wordbegin, wordend);
    for (p = 0; p < nrecord; p++)
        printf("internal %d=%23.16E %23.16E\n", p, drecord[p] / 86400E0, drecord[p]);
    printf("recT=%d\n", recT);
    printf("TL=%23.16E\n", arecord.tl);
    for (k = 0; k < nvalid; k++)
        printf("g[%d]=%23.16E\n", k, arecord.g[k]);
    for (k = 0; k < 3; k++)
        printf("refpos[%d]=%23.16E\n", k, arecord.refpos[k]);
    for (k = 0; k < 3; k++)
        printf("refvel[%d]=%23.16E\n", k, arecord.refvel[k]);
    for (p = 0; p < nvalid; p++)
    {
        for (k = 0; k < 3; k++)
            printf("dt[%d][%d]=%23.16E\n", p, k, arecord.dt[p][k]);
    }
    printf("kqmax1=%d\n", arecord.kqmax1);
    for (k = 0; k < 3; k++)
        printf("kq[%d]=%d\n", k, arecord.kq[k]);
    if (recT >= seg->seginfo.data21.count_record)
        fatalerror("calceph_spk_interpol_PV_segment_21 cas 2 : %d %d\n", recT, nrecord);
    if (subdir >= seg->seginfo.data21.count_record)
        fatalerror("calceph_spk_interpol_PV_segment_21 cas 1 : %d %d\n", subdir, nrecord);

    calceph_stateType_debug(Planet);
#endif

    return 1;
}
