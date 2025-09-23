/*-----------------------------------------------------------------*/
/*! 
  \file calcephspkseg13.c 
  \brief perform I/O on SPICE KERNEL ephemeris data file 
         for the segments of type 13
         interpolate SPICE KERNEL Ephemeris data.
         for the segments of type 13
         compute position and velocity from SPICE KERNEL ephemeris file
         for the segments of type 13.

  \author  M. Gastineau 
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de Paris. 

   Copyright, 2016-2024, CNRS
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
/*!                                                      
     This function read the data block corresponding to the specified Time
     if this block isn't already in memory.
     
     This function works only over the specified segment.
     The segment must be of type 9 or 13 (seg->seginfo.data13 must be valid).
     
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
int calceph_spk_interpol_PV_segment_13(struct SPKfile *pspk, struct SPKSegmentHeader *seg, struct SPICEcache *cache,
                                       treal TimeJD0, treal Timediff, stateType * Planet)
{
    int p;

    const double Timesec = ((TimeJD0 - 2.451545E+06) + Timediff) * 86400E0;

    int recT;                   /* record where is located Timesec */

    const double *drecord;

    int nrecord;

    int subdir = 0;

    int wordbegin, wordend;

    int interpbegin, interpend;

    int S;

    double depoch[1000];

    /* read all the epoch */
    int nreadepoch = seg->seginfo.data13.count_record;

    if (nreadepoch > 100)
    {
        /* locate the correct nepoch slot using the fast directory */
        while (subdir < seg->seginfo.data13.count_directory && seg->seginfo.data13.directory[subdir] < Timesec)
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
        drecord = seg->seginfo.data13.directory;
        nrecord = nreadepoch;
    }

    /* find the record */
    recT = 0;
    while (recT < nrecord - 1 && drecord[recT] < Timesec)
        recT++;
    recT += subdir;

    /* compute the bound for the interpolation */
    S = seg->seginfo.data13.window_sizem1 + 1;
    if (S % 2 == 0)
    {
        interpbegin = recT - S / 2;
        interpend = recT + (S / 2 - 1);
    }
    else
    {
        interpbegin = recT - (S - 1) / 2;
        interpend = recT + (S - 1) / 2;
    }
    /* check if the bound are outside the records */
    if (interpbegin < 0)
    {
        interpbegin = 0;
        interpend = S - 1;
    }
    if (interpend >= nreadepoch)
    {
        interpend = nreadepoch - 1;
        interpbegin = interpend - S + 1;
    }

    /* load the record */
    for (p = 0; p < S; p++)
        depoch[p] = drecord[interpbegin + p - subdir];
    if (calceph_spk_fastreadword
        (pspk, seg, cache, "", seg->rec_begin + 6 * interpbegin, seg->rec_begin + 6 * interpend + 5, &drecord) == 0)
    {
        return 0;
    }

    switch (seg->datatype)
    {
        case SPK_SEGTYPE9:
            calceph_spk_interpol_Lagrange(S, drecord, depoch, (TimeJD0 - 2.451545E+06) * 86400E0, Timediff * 86400E0,
                                          Planet);
            break;
        case SPK_SEGTYPE13:
            calceph_spk_interpol_Hermite(S, drecord, depoch, (TimeJD0 - 2.451545E+06) * 86400E0, Timediff * 86400E0,
                                         Planet);
            break;

        default:
            /* GCOVR_EXCL_START */
            fatalerror("Internal error: Unsupported segment (type=%d).\n", (int) seg->datatype);
            /* GCOVR_EXCL_STOP */
            break;
    }

#if DEBUG
    /* print the record */
    printf("S=%d %d\n", S, seg->seginfo.data13.window_sizem1 + 1);
    printf("S2=%d %d\n", S, (interpend - interpbegin + 1));
    printf("Timesec=%23.16E\n", Timesec);
    printf("seg->seginfo.data13.count_record : %d\n", seg->seginfo.data13.count_record);
    printf("interpbegin=%d interpend=%d\n", interpbegin, interpend);
    printf("subdir=%d\n", subdir);
    printf("read : %d %d\n", wordbegin, wordend);
    for (p = 0; p < 6 * S; p++)
        printf("internal %d [%23.16E(=%23.16E)] %23.16E\n", p, depoch[p / 6] / 86400E0, depoch[p / 6], drecord[p]);
    printf("recT=%d\n", recT);
    if (interpbegin < 0)
        fatalerror("calceph_spk_interpol_PV_segment_13 cas 2 : %d %d\n", recT, nrecord);
    if (interpend >= seg->seginfo.data13.count_record)
        fatalerror("calceph_spk_interpol_PV_segment_13 cas 2 : %d %d\n", recT, nrecord);
    if (recT >= seg->seginfo.data13.count_record)
        fatalerror("calceph_spk_interpol_PV_segment_13 cas 2 : %d %d\n", recT, nrecord);
    if (subdir >= seg->seginfo.data13.count_record)
        fatalerror("calceph_spk_interpol_PV_segment_13 cas 1 : %d %d\n", subdir, nrecord);

    calceph_stateType_debug(Planet);
#endif

    return 1;
}
