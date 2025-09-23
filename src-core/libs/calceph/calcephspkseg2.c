/*-----------------------------------------------------------------*/
/*! 
  \file calcephspkseg2.c 
  \brief perform I/O on SPICE KERNEL ephemeris data file 
         for the segments of type 2/3/20
         interpolate SPICE KERNEL Ephemeris data.
         for the segments of type 2/3/20
         compute position and velocity from SPICE KERNEL ephemeris file
         for the segments of type 2/3/20.

  \author  M. Gastineau 
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de Paris. 

   Copyright, 2011-2023, CNRS
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
#include "calcephchebyshev.h"

static void calceph_spk_interpol_PV_segment_20_an(const double *Coeff_Array, treal TimeJD0,
                                                  treal Timediff, struct SPKSegmentHeader_20 *seg,
                                                  treal Timerecordbeg, stateType * Planet, int N);

/*--------------------------------------------------------------------------*/
/*!                                                      
     This function read the data block corresponding to the specified Time
     if this block isn't already in memory.
     
     This function works only over the specified segment.
     The segment must be of type 2, 3, 102 or 103.
     
     It computes position and velocity vectors for a selected  
     planetary body from Chebyshev coefficients.          

                                                                          
  @param pspk (inout) SPK file  
    pspsk is not updated if the data are already prefetched.
  @param seg (in) segment where is located TimeJD0+timediff 
  @param cache (inout) cache for the object
  @param TimeJD0  (in) Time for which position is desired (Julian Date)
  @param Timediff  (in) offset time from TimeJD0 for which position is desired (Julian Date)
  @param Planet (out) position and velocity                                                                        
         Planet has position in km                                                
         Planet has velocity in km/sec
*/
/*--------------------------------------------------------------------------*/
int calceph_spk_interpol_PV_segment_2(struct SPKfile *pspk, struct SPKSegmentHeader *seg, struct SPICEcache *cache,
                                      treal TimeJD0, treal Timediff, stateType * Planet)
{
    const double Timesec = ((TimeJD0 - 2.451545E+06) + Timediff) * 86400E0;

    int recT;                   /* record where is located Timesec */

    const double *drecord;

    int wordT;

    const int nword = seg->seginfo.data2.count_dataperrecord;

#if DEBUG
    int j;
#endif
    int ncomp, N;

    recT = (Timesec - seg->seginfo.data2.T_begin) / seg->seginfo.data2.T_len;
    if (recT == seg->seginfo.data2.count_record &&
        Timesec <= seg->seginfo.data2.T_begin + seg->seginfo.data2.T_len * recT)
        recT = recT - 1;

    if (recT < 0 || recT >= seg->seginfo.data2.count_record)
    {
        fatalerror
            ("Computation of record is not valid for segment of type 2. Looking for time %23.16E. Beginning time in segment : %23.16E\n"
             "Time slice in the segment : %23.16E\n. Number of records: %d\n Coumputed record : %d\n", Timesec,
             seg->seginfo.data2.T_begin, seg->seginfo.data2.T_len, seg->seginfo.data2.count_record, recT);
        return 0;
    }

    /* read the data */
    wordT = (seg->rec_begin + recT * nword);
    if (calceph_spk_fastreadword(pspk, seg, cache, "", wordT, wordT + nword - 1, &drecord) == 0)
    {
        return 0;
    }
#if DEBUG
    for (j = 0; j < nword; j++)
        printf("%23.16E\n", drecord[j]);
    printf
        ("seg->seginfo.data2.T_begin=%23.16E seg->seginfo.data2.T_len=%23.16E begin record=%23.16E end record =%23.16E Timesec=%23.16E\n",
         seg->seginfo.data2.T_begin - seg->seginfo.data2.T_len, seg->seginfo.data2.T_begin + seg->seginfo.data2.T_len,
         drecord[0], drecord[0] + drecord[1], Timesec);
#endif

    /* computation */
    ncomp = (seg->datatype == SPK_SEGTYPE3 || seg->datatype == SPK_SEGTYPE103) ? 6 : 3;
    N = (nword - 2) / ncomp;

    calceph_spk_interpol_Chebychev(N, ncomp, drecord, (TimeJD0 - 2.451545E+06) * 86400E0, Timediff * 86400E0, Planet);

#if DEBUG
    printf("ncomp=%d %d\n", ncomp, N);
    calceph_stateType_debug(Planet);
#endif

    return 1;
}

/*--------------------------------------------------------------------------*/
/*!  This function computes position and velocity vectors                 
     from Chebyshev coefficients from Coeff_Array.                                      
                                          
                                                                          
 @param Coeff_Array (in) array of Tchebychev coefficient                         
 @param TimeJD0  (in) Time for which position is desired (Julian Date)
 @param Timediff  (in) offset time from TimeJD0 for which position is desired (Julian Date)
 @param Planet (out) Pointer to external array to receive the position/velocity.
   Planet has position in ephemerides units                                 
   Planet has velocity in ephemerides units                                 
 @param N(in)    Number of coeff's      
 @param ncomp (in) number of component
*/
/*--------------------------------------------------------------------------*/
void calceph_spk_interpol_Chebychev(int N, int ncomp, const double *Coeff_Array, treal TimeJD0,
                                    treal Timediff, stateType * Planet)
{
    treal Tc, scale;

    long long int TimeJD0pent = longr(TimeJD0);

    treal TimeJD0frac = TimeJD0 - realr(TimeJD0pent);

    long long int Timediffpent = longr(Timediff);

    treal Timedifffrac = Timediff - realr(Timediffpent);

    long long int T_begpent;

    treal T_begfrac;

    treal deltapent, deltafrac;

    stateType X;

    const double T_beg = Coeff_Array[0] - Coeff_Array[1];

    const double T_span = Coeff_Array[1] * 2.E0;

    const double *A = Coeff_Array + 2;

    X.order = Planet->order;
  /*--------------------------------------------------------------------------*/
    /* Compute Tc                           */
  /*--------------------------------------------------------------------------*/

    T_begpent = longr(T_beg);
    T_begfrac = T_beg - T_begpent;
    /* code equivalent a :
       Tc = 2.0E0*(Time - T_beg) / T_span - 1.0E0; */
    deltapent = (TimeJD0pent + Timediffpent - T_begpent) / T_span;
    deltafrac = (TimeJD0frac + Timedifffrac - T_begfrac) / T_span;
    Tc = REALC(2.0E0) * (deltapent + deltafrac) - REALC(1.0E0);

  /*--------------------------------------------------------------------------*/
    /* Compute the interpolated position and velocity                           */
  /*--------------------------------------------------------------------------*/
    scale = REALC(2.0E0) / (T_span);

    calceph_interpol_PV_lowlevel(&X, A, Tc, scale, N, ncomp);

  /*--------------------------------------------------------------------------*/
    /*  Return computed values.                                                 */
  /*--------------------------------------------------------------------------*/

    *Planet = X;

}

/*--------------------------------------------------------------------------*/
/*!                                                      
     This function read the data block corresponding to the specified Time
     if this block isn't already in memory.
     
     This function works only over the specified segment.
     The segment must be of type 20 or 120.
     
     It computes position and velocity vectors for a selected  
     planetary body from Chebyshev coefficients.          

                                                                          
  @param pspk (inout) SPK file  
    pspsk is not updated if the data are already prefetched.
  @param seg (in) segment where is located TimeJD0+timediff 
  @param cache (inout) cache for the object
  @param TimeJD0  (in) Time for which position is desired (Julian Date)
  @param Timediff  (in) offset time from TimeJD0 for which position is desired (Julian Date)
  @param Planet (out) position and velocity                                                                        
         Planet has position in ua                                                
         Planet has velocity in ua/day
*/
/*--------------------------------------------------------------------------*/
int calceph_spk_interpol_PV_segment_20(struct SPKfile *pspk, struct SPKSegmentHeader *seg, struct SPICEcache *cache,
                                       treal TimeJD0, treal Timediff, stateType * Planet)
{

    int recT;                   /* record where is located Timesec */

    const double *drecord;

    int wordT;

    const int nword = seg->seginfo.data20.count_dataperrecord;

#if DEBUG
    int j;
#endif
    int ncomp, N;

    const double tbegin = (TimeJD0 - seg->seginfo.data20.T_init_JD) + (Timediff - seg->seginfo.data20.T_init_FRACTION);

    const double Timesec = tbegin * 86400E0;

    recT = Timesec / seg->seginfo.data20.T_len;
#if DEBUG
    printf("recT=%d tbegin=%g Timesec=%g\n", recT, tbegin, Timesec);
#endif
    if (recT == seg->seginfo.data20.count_record && Timesec <= seg->seginfo.data20.T_len * recT)
        recT = recT - 1;

    if (recT < 0 || recT >= seg->seginfo.data20.count_record)
    {
        fatalerror
            ("Computation of record is not valid for segment of type 20. Looking for time %23.16E. Beginning time in segment : %23.16E\n"
             "Time slice in the segment : %23.16E\n. Number of records: %d\n Coumputed record : %d\n", Timesec,
             seg->seginfo.data20.T_begin, seg->seginfo.data20.T_len, seg->seginfo.data20.count_record, recT);
        return 0;
    }

    /* read the data */
    wordT = (seg->rec_begin + recT * nword);

    if (calceph_spk_fastreadword(pspk, seg, cache, "", wordT, wordT + nword - 1, &drecord) == 0)
    {
        return 0;
    }
#if  DEBUG
    for (j = 0; j < nword; j++)
        printf("%23.16E\n", drecord[j]);
    printf
        ("seg->seginfo.data20.T_begin=%23.16E seg->seginfo.data2.T_len=%23.16E begin record=%23.16E end record =%23.16E Timesec=%23.16E\n",
         seg->seginfo.data20.T_begin, seg->seginfo.data20.T_len,
         tbegin + seg->seginfo.data20.T_len * recT,
         tbegin + seg->seginfo.data20.T_len * recT + seg->seginfo.data20.T_len, Timesec);
#endif

    /* computation */
    ncomp = 3;
    N = (nword - 2) / ncomp;

    calceph_spk_interpol_PV_segment_20_an(drecord, TimeJD0, Timediff,
                                          &seg->seginfo.data20, seg->seginfo.data20.T_len * recT, Planet, N);

#if DEBUG
    calceph_stateType_debug(Planet);
#endif

    return 1;
}

/*--------------------------------------------------------------------------*/
/*!  This function computes position and velocity vectors                 
     from Chebyshev coefficients from Coeff_Array.                                      
       Coeff_Array must be compliant with format of type 20                                   
                                                                          
 @param Coeff_Array (in) array of Tchebychev coefficient : only veclocity followed by position at t=0                         
 @param TimeJD0  (in) Time for which position is desired (Julian Date)
 @param Timediff  (in) offset time from TimeJD0 for which position is desired (Julian Date)
 @param seg (in) time ad distance information for the record
 @param Planet (out) Pointer to external array to receive the position/velocity.
   Planet has position in ephemerides units                                 
   Planet has velocity in ephemerides units                                 
 @param N(in)    Number of coeff's      
*/
/*--------------------------------------------------------------------------*/
static void calceph_spk_interpol_PV_segment_20_an(const double *Coeff_Array, treal TimeJD0,
                                                  treal Timediff, struct SPKSegmentHeader_20 *seg,
                                                  treal Timerecordbeg, stateType * Planet, int N)
{

#if __STDC_VERSION__>199901L
#define NSTACK_INTERPOL (N+1)
#else
#define NSTACK_INTERPOL 15000
#endif

    treal Cp[NSTACK_INTERPOL + 1], P_Sum[3], V_Sum[3], Tc, Ip[NSTACK_INTERPOL], Up[NSTACK_INTERPOL],
        Vp[NSTACK_INTERPOL];

#undef NSTACK_INTERPOL

    long long int TimeJD0pent = longr(TimeJD0);

    treal TimeJD0frac = TimeJD0 - realr(TimeJD0pent);

    long long int Timediffpent = longr(Timediff);

    treal Timedifffrac = Timediff - realr(Timediffpent);

    long long int T_begpent;

    treal T_begfrac;

    treal deltapent, deltafrac;

    treal scaleder, DoTscale;

    int i, j;

    int flag;

    double d;

    stateType X;

    const double *A = Coeff_Array;

    const treal Dscale = seg->dscale;

    const treal Tscale = seg->tscale;

    const treal T_span = seg->T_len;

    X.order = Planet->order;
  /*--------------------------------------------------------------------------*/
    /* Compute Tc                           */
  /*--------------------------------------------------------------------------*/

    T_begpent = seg->T_init_JD * 86400E0 + Timerecordbeg;
    T_begfrac = seg->T_init_FRACTION * 86400E0;
    /* code equivalent a :
       Tc = 2.0E0*(Time - T_beg) / T_span - 1.0E0; */
    deltapent = ((TimeJD0pent * REALC(86400E0) - T_begpent) + Timediffpent * REALC(86400E0)) / T_span;
    deltafrac = ((TimeJD0frac * REALC(86400E0) - T_begfrac) + Timedifffrac * REALC(86400E0)) / T_span;
    Tc = REALC(2.0E0) * (deltapent + deltafrac) - REALC(1.0E0);
#if DEBUG
    printf("Tc=%23.16E %g %g %23.16E\n", Tc, TimeJD0, Timediff, Dscale);
#endif

    /*--------------------------------------------------------------------------*/
    /* Compute the interpolated velocity                           */
    /*--------------------------------------------------------------------------*/
    calceph_chebyshev_order_0(Cp, N + 1, Tc);

    Ip[0] = Tc;
    Ip[1] = (Cp[2] + Cp[0]) / (REALC(4.0E0));

    for (j = 2; j < N; j++)
    {
        Ip[j] = (Cp[j + 1] / (j + 1) - Cp[j - 1] / (j - 1)) / (REALC(2.0E0));
    }

    /* remove constant term from the integral */
    flag = 0;

    d = REALC(2.0E0) * Tc;
    for (i = 3, j = 1; i < N; i += 2, j++)
    {
        d = REALC(0.25E0) / j + REALC(0.25E0) / (j + 1);
        flag = 1 - flag;
        if (flag)
        {
            d = -d;
        }
        Ip[i] += d;
    }

    DoTscale = Dscale / Tscale;
    for (i = 0; i < 3; i++)     /* Compute interpolating polynomials */
    {
#if DEBUG
        printf("utilisation des coef suivants i=%d\n", i);
        for (j = N - 1; j > -1; j--)
            printf("%23.16E %23.16E \n", A[j + i * (N + 1)], Ip[j]);
        printf("position = %23.16E\n", A[(i + 1) * (N + 1) - 1]);
#endif
        P_Sum[i] = REALC(0.0E0);    /* Compute interpolated position */
        for (j = N - 1; j > -1; j--)
        {
            P_Sum[i] = P_Sum[i] + A[j + i * (N + 1)] * Ip[j];
        }
        P_Sum[i] = REALC(0.5E0) * P_Sum[i] * (T_span / Tscale) + A[(i + 1) * (N + 1) - 1];

        X.Position[i] = P_Sum[i] * Dscale;

        /* Compute interpolated velocity */
        if (X.order >= 1)
        {
            V_Sum[i] = REALC(0.0E0);    /* Compute interpolated velocity */

            for (j = N - 1; j > -1; j--)
            {
                V_Sum[i] = V_Sum[i] + A[j + i * (N + 1)] * Cp[j];
            }
            X.Velocity[i] = V_Sum[i] * DoTscale;
        }

    }

    /* Compute interpolated acceleration */
    scaleder = REALC(0.5E0) * T_span;
    if (X.order >= 2)
    {
        calceph_chebyshev_order_1(Up, N, Tc, Cp);
        calceph_interpolate_chebyshev_order_1_stride_n(X.Acceleration, N, Up, A, N + 1, DoTscale / scaleder, 0);
    }

    /* Compute interpolated jerk */
    if (X.order >= 3)
    {
        calceph_chebyshev_order_2(Vp, N, Tc, Up);
        calceph_interpolate_chebyshev_order_2_stride_n(X.Jerk, N, Vp, A, N + 1, DoTscale / (scaleder * scaleder), 0);
    }

  /*--------------------------------------------------------------------------*/
    /*  Return computed values.                                                 */
  /*--------------------------------------------------------------------------*/

    *Planet = X;

}
