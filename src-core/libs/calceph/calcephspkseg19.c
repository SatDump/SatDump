/*-----------------------------------------------------------------*/
/*!
  \file calcephspkseg19.c
  \brief perform I/O on SPICE KERNEL ephemeris data file
         for the segments of type 19
         interpolate SPICE KERNEL Ephemeris data.
         for the segments of type 19
         compute position and velocity from SPICE KERNEL ephemeris file
         for the segments of type 19.

  \author  M. Gastineau
           Astronomie et Systemes Dynamiques, LTE, CNRS, Observatoire de Paris.

   Copyright, 2025, CNRS
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
/*! read the data header of this segment of type 19
   @param file (inout) file descriptor.
   @param filename (in) A character string giving the name of an ephemeris data
   file.
   @param seg (inout) segment
*/
/*--------------------------------------------------------------------------*/
int calceph_spk_readsegment_header_19(FILE *file, const char *filename, struct SPKSegmentHeader *seg)
{
    double drecord[12];

    int res = 1;

    res = calceph_spk_readword(file, filename, seg->rec_end - 1, seg->rec_end, drecord);
    if (res == 1)
    {
        int ndirectory, n;

        calceph_bff_convert_array_double(drecord, 2, seg->bff);

        seg->seginfo.data19.boundary_choice_flag = (int) drecord[0];
        seg->seginfo.data19.number_intervals = (int) drecord[1];
        seg->seginfo.data19.mini_segments = NULL;
        seg->seginfo.data19.boundary_times_directory = NULL;
        seg->seginfo.data19.boundary_times = NULL;
        n = seg->seginfo.data19.number_intervals;
        ndirectory = (n > 100) ? (n - 1) / 100 : 0;
        seg->seginfo.data19.count_boundary_times_directory = ndirectory;
        res = calceph_spk_allocate_directory(ndirectory, &seg->seginfo.data19.boundary_times_directory);
        if (res == 1)
            res = calceph_spk_allocate_directory(n + 1, &seg->seginfo.data19.boundary_times);
        if (res == 1)
            res = calceph_spk_allocate_directory(n + 1, &seg->seginfo.data19.mini_segments);
        if (res == 1)
        {
            int prev_end = seg->rec_end - 2;
            int next_end = prev_end - n;

            res = calceph_spk_readword(file, filename, next_end, prev_end, seg->seginfo.data19.mini_segments);
            calceph_bff_convert_array_double(seg->seginfo.data19.mini_segments, n + 1, seg->bff);
            prev_end = next_end - 1;

            if (ndirectory > 0)
            {
                next_end = prev_end - ndirectory + 1;
                res =
                    calceph_spk_readword(file, filename, next_end, prev_end,
                                         seg->seginfo.data19.boundary_times_directory);
                calceph_bff_convert_array_double(seg->seginfo.data19.boundary_times_directory, ndirectory, seg->bff);
#if DEBUG
                printf("ndirectory=%d %23.16E %23.16E\n", ndirectory, seg->seginfo.data19.boundary_times_directory[0],
                       seg->seginfo.data19.boundary_times_directory[ndirectory - 1]);
                for (int j = 0; j < ndirectory; j++)
                    printf("read subdir=%d %23.16E\n", j, seg->seginfo.data19.boundary_times_directory[j]);
#endif
                prev_end = next_end - 1;
            }
            next_end = prev_end - n;
            res = calceph_spk_readword(file, filename, next_end, prev_end, seg->seginfo.data19.boundary_times);
            calceph_bff_convert_array_double(seg->seginfo.data19.boundary_times, n + 1, seg->bff);
        }
    }

    return res;
}

/*--------------------------------------------------------------------------*/
/*!
     This function read the data block corresponding to the specified Time
     if this block isn't already in memory.

     This function works only over the specified segment.
     The segment must be of type 19 (seg->seginfo.data19 must be valid).

     It computes position and velocity vectors for a selected
     planetary body from Chebyshev coefficients.

  @param pspk (inout) SPK file
  @param seg (in) segment where is located TimeJD0+timediff
  @param cache (inout) cache for the object
  @param TimeJD0  (in) Time for which position is desired (Julian Date)
  @param Timediff  (in) offset time from TimeJD0 for which position is desired (Julian Date)
  @param Planet (out) position and velocity
         Planet has position in km
         Planet has velocity in km/sec
*/
/*--------------------------------------------------------------------------*/
int calceph_spk_interpol_PV_segment_19(struct SPKfile *pspk, struct SPKSegmentHeader *seg,
                                       struct SPICEcache *cache, treal TimeJD0, treal Timediff, stateType *Planet)
{
    int res = 1;

    struct SPKSegmentHeader mini_seg18;

    const double Timesec = ((TimeJD0 - 2.451545E+06) + Timediff) * 86400E0;

    int recT;                   /* record where is located Timesec */

    const double *drecord;

    int nrecord;

    int subdir = 0;

    /* read all the epoch */
    int nreadepoch = seg->seginfo.data19.number_intervals;

    if (nreadepoch > 100)
    {
        /* locate the correct nepoch slot using the fast directory */
        while (subdir < seg->seginfo.data19.count_boundary_times_directory &&
               seg->seginfo.data19.boundary_times_directory[subdir] < Timesec)
        {
            subdir++;
        }
        subdir *= 100;
        if (subdir > 0)
            subdir--;
        drecord = seg->seginfo.data19.boundary_times + subdir;
        nrecord = 100;
        if (subdir + nrecord >= nreadepoch)
            nrecord = nreadepoch - subdir;
    }
    else
    {                           /* less than 100 epochs */
        drecord = seg->seginfo.data19.boundary_times;
        nrecord = nreadepoch;
    }

    /* find the record */
    recT = 0;
    while (recT < nrecord - 1 && drecord[recT + 1] < Timesec)
        recT++;
    recT += subdir;
    if (seg->seginfo.data19.boundary_choice_flag == 0)
    {
        if (seg->seginfo.data19.boundary_times[recT] == Timesec && recT > 0)
            recT--;
    }

    mini_seg18.rec_begin = seg->rec_begin + seg->seginfo.data19.mini_segments[recT] - 1;
    mini_seg18.rec_end = seg->rec_begin + seg->seginfo.data19.mini_segments[recT + 1] - 2;
    mini_seg18.bff = seg->bff;
    mini_seg18.datatype = 18;

#if DEBUG
    /* print the record */
    printf("in segment 19 : recT=%d  number_intervals = %d\n", recT, seg->seginfo.data19.number_intervals);
    printf("Timesec=%23.16E\n", Timesec);
    printf("timebegin=%23.16E timeend=%23.16E\n", seg->seginfo.data19.boundary_times[recT],
           seg->seginfo.data19.boundary_times[recT + 1]);
    printf("interpbegin=%d interpend=%d\n", mini_seg18.rec_begin, mini_seg18.rec_end);
    printf("subdir=%d\n", subdir);
#endif

    res = calceph_spk_readsegment_header_18(pspk->file, "", &mini_seg18);
    if (res == 1)
    {
        res = calceph_spk_interpol_PV_segment_18(pspk, &mini_seg18, cache, TimeJD0, Timediff, Planet);
        free(mini_seg18.seginfo.data18.directory);
    }
    return res;
}
