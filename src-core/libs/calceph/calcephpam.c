/*-----------------------------------------------------------------*/
/*!
  \file calcephpam.c
  \brief perform I/O and several computations for Angular Momentum of the planets (rotating body)
         for inpop ephemeris only.

  \author  M. Gastineau
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de Paris.

   Copyright, 2017-2024, CNRS
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
#ifndef __cplusplus
#if HAVE_STDBOOL_H
#include <stdbool.h>
#endif
#endif /*__cplusplus */
#if HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include "calcephdebug.h"
#include "real.h"
#define __CALCEPH_WITHIN_CALCEPH 1
#include "calceph.h"
#include "calcephinternal.h"
#include "util.h"

#if DEBUG
static void calceph_debug_pam(const t_pam_calcephbin * infopam);
#endif

static int calceph_interpol_PV_pam(t_pam_calcephbin * p_pbinfile, treal TimeJD0, treal Timediff,
                                   int Target, int *ephunit, int id, stateType * Planet);

/*--------------------------------------------------------------------------*/
/*!  Initialize to "NULL" all fields of p_ppameph.

  @param p_ppameph (out) Angular Momentum of the planets ephemeris.
*/
/*--------------------------------------------------------------------------*/
void calceph_empty_pam(t_pam_calcephbin * p_ppameph)
{
    p_ppameph->inforec.locNextRecord = 0;
    p_ppameph->inforec.numAngMomPlaRecord = 0;
    p_ppameph->inforec.nCompAngMomPla = 0;
    p_ppameph->inforec.locCoeffAngMomPlaRecord = 0;
    p_ppameph->inforec.sizeAngMomPlaRecord = 0;
    p_ppameph->coefftime_array.file = NULL;
    p_ppameph->coefftime_array.Coeff_Array = NULL;
    p_ppameph->coefftime_array.mmap_buffer = NULL;
    p_ppameph->coefftime_array.mmap_size = 0;
    p_ppameph->coefftime_array.mmap_used = 0;
    p_ppameph->coefftime_array.prefetch = 0;
}

/*--------------------------------------------------------------------------*/
/*!  free memory used by  p_ppameph.

  @param p_ppameph (inout) Angular Momentum of the planets ephemeris.
*/
/*--------------------------------------------------------------------------*/
void calceph_free_pam(t_pam_calcephbin * p_ppameph)
{
    if (p_ppameph->coefftime_array.prefetch == 0)
    {
        if (p_ppameph->coefftime_array.Coeff_Array != NULL)
            free(p_ppameph->coefftime_array.Coeff_Array);
    }
    /* the prefetch fields are not freed because there are shared with the planetary records */
    calceph_empty_pam(p_ppameph);
}

/*--------------------------------------------------------------------------*/
/*!  Initialize  p_ppameph.
     Read records from the file.
  @return returns 1 on success. return 0 on failure.

  @param p_ppameph (inout) Angular Momentum of the planets  ephemeris.
  @param file (inout) binary ephemeris file
  @param swapbyte (in) =0 => don't swap byte order of the data
  @param timeData (in) begin/end/step of the time in the file
  @param recordsize (in) size of a record in number of coefficients
  @param locnextrecord (inout) on enter, location to read the Angular Momentum of the planets record
            on exit, the location of the next record (could be 0 if no other record)
*/
/*--------------------------------------------------------------------------*/
int calceph_init_pam(t_pam_calcephbin * p_ppameph, FILE * file, int swapbyte, double timeData[3],
                     int recordsize, int *locnextrecord)
{
    size_t recordsizebytes = sizeof(double) * recordsize;

    off_t offset;

    int j, k;

    buffer_error_t buffer_error;

    p_ppameph->coefftime_array.file = file;
    p_ppameph->coefftime_array.swapbyteorder = swapbyte;
    p_ppameph->coefftime_array.Coeff_Array = NULL;

    /*--------------------------------------------------------------------------*/
    /* read the Information PAM record */
    /*--------------------------------------------------------------------------*/
    offset = (*locnextrecord) - 1;

#if DEBUG
    printf("read Information PAM record at slot %lu\n", (unsigned long) offset);
#endif
    if (fseeko(file, offset * recordsizebytes, SEEK_SET) != 0)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't jump to Information Planetary Angular Momentum record \nSystem error : '%s'\n",
                   calceph_strerror_errno(buffer_error));
        /* GCOVR_EXCL_STOP */
        return 0;
    }
    if (fread(&p_ppameph->inforec, sizeof(t_InfoPamRecord), 1, file) != 1)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't read Information Planetary Angular Momentum record\n");
        /* GCOVR_EXCL_STOP */
        return 0;
    }

    /*swap */
    if (swapbyte)
    {
        p_ppameph->inforec.locNextRecord = swapint(p_ppameph->inforec.locNextRecord);
        p_ppameph->inforec.numAngMomPlaRecord = swapint(p_ppameph->inforec.numAngMomPlaRecord);
        p_ppameph->inforec.nCompAngMomPla = swapint(p_ppameph->inforec.nCompAngMomPla);
        p_ppameph->inforec.locCoeffAngMomPlaRecord = swapint(p_ppameph->inforec.locCoeffAngMomPlaRecord);
        p_ppameph->inforec.sizeAngMomPlaRecord = swapint(p_ppameph->inforec.sizeAngMomPlaRecord);
        for (j = 0; j < 12; j++)
        {
            for (k = 0; k <= 2; k++)
            {
                p_ppameph->inforec.coeffPtr[j][k] = swapint(p_ppameph->inforec.coeffPtr[j][k]);
            }
        }
    }
    p_ppameph->coefftime_array.ncomp = p_ppameph->inforec.nCompAngMomPla;

#if DEBUG
    calceph_debug_pam(p_ppameph);
#endif

    if (p_ppameph->inforec.numAngMomPlaRecord > 0)
    {
        /*--------------------------------------------------------------------------*/
        /* allocate memory for coefficient record */
        /*--------------------------------------------------------------------------*/
        p_ppameph->coefftime_array.ARRAY_SIZE = p_ppameph->inforec.sizeAngMomPlaRecord;
        p_ppameph->coefftime_array.Coeff_Array
            = (double *) malloc(sizeof(double) * p_ppameph->coefftime_array.ARRAY_SIZE);
        if (p_ppameph->coefftime_array.Coeff_Array == NULL)
        {
            /* GCOVR_EXCL_START */
            fatalerror("Can't allocate memory for %d reals\nSystem error : '%s'\n",
                       p_ppameph->coefftime_array.ARRAY_SIZE, calceph_strerror_errno(buffer_error));
            calceph_free_pam(p_ppameph);
            return 0;
            /* GCOVR_EXCL_STOP */
        }
        /*--------------------------------------------------------------------------*/
        /* read the first coefficient PAM record */
        /*--------------------------------------------------------------------------*/
        p_ppameph->coefftime_array.offfile
            = (off_t) (p_ppameph->inforec.locCoeffAngMomPlaRecord - 1) * (off_t) recordsizebytes;
        if (fseeko(file, p_ppameph->coefftime_array.offfile, SEEK_SET) != 0)
        {
            /* GCOVR_EXCL_START */
            fatalerror("Can't jump to the Coefficient  Planetary Angular Momentum record \nSystem error : "
                       "'%s'\n", calceph_strerror_errno(buffer_error));
            calceph_free_pam(p_ppameph);
            return 0;
            /* GCOVR_EXCL_STOP */
        }
        if (!calceph_inpop_readcoeff(&p_ppameph->coefftime_array, timeData[0]))
        {
            calceph_free_pam(p_ppameph);
            return 0;
        }
    }

    *locnextrecord = p_ppameph->inforec.locNextRecord;
    return 1;
}

#if DEBUG
/*--------------------------------------------------------------------------*/
/*!  debug the structure t_pam_calcephbin.

  @param p_ppameph (in) Angular Momentum of the planets  ephemeris.
*/
/*--------------------------------------------------------------------------*/
static void calceph_debug_pam(const t_pam_calcephbin * p_ppameph)
{
    int j;

    printf("PAM locNextRecord: %d\n", p_ppameph->inforec.locNextRecord);
    printf("numAngMomPlaRecord: %d\n", p_ppameph->inforec.numAngMomPlaRecord);
    printf("nCompAngMomPla: %d\n", p_ppameph->inforec.nCompAngMomPla);
    printf("locCoeffAngMomPlaRecord: %d\n", p_ppameph->inforec.locCoeffAngMomPlaRecord);
    printf("sizeAngMomPlaRecord: %d\n", p_ppameph->inforec.sizeAngMomPlaRecord);
    for (j = 0; j < 12; j++)
    {
        printf("PAM body=%d location=%d ncoeff=%d granule=%d\n", j, p_ppameph->inforec.coeffPtr[j][0],
               p_ppameph->inforec.coeffPtr[j][1], p_ppameph->inforec.coeffPtr[j][2]);
    }
}
#endif /*DEBUG*/
/*--------------------------------------------------------------------------*/
/*!
     This function read the data block corresponding to the specified Time
     if this block isn't already in memory.

     This function works only for the G/(mR^2).

     It computes position and velocity vectors for a selected
     planetary body from Chebyshev coefficients.

  @param p_pbinfile (inout) ephemeris descriptor for G/(mR^2)
       The data of p_pbinfile is not updated if data are prefetched (prefetch!=0) 
  @param TimeJD0  (in) Time for which position is desired (Julian Date)
  @param Timediff  (in) offset time from TimeJD0 for which position is desired (Julian Date)
  @param Target   (in) Solar system body for which position is desired.
  @param Planet (out) position and velocity
         Planet has position in ua
         Planet has velocity in ua/day
  @param isinAU (in) = 1 if unit in the file are in AU
  @param ephunit (out) units of the output state
  @param loccoeff (in) location of the coefficients in the array t_pam_calcephbin.inforec.coeffPtr
*/
/*--------------------------------------------------------------------------*/
static int calceph_interpol_PV_pam(t_pam_calcephbin * p_pbinfile, treal TimeJD0, treal Timediff,
                                   int Target, int *ephunit, int id, stateType * Planet)
{
    treal Time = TimeJD0 + Timediff;

    int C, G, N, ncomp;

    t_memarcoeff localmemarrcoeff;  /* local data structure for thread-safe */

    /*--------------------------------------------------------------------------*/
    /* check if the file contains rotational angular momentum.                */
    /*--------------------------------------------------------------------------*/
    if (p_pbinfile->coefftime_array.file == NULL)
    {
        fatalerror("The ephemeris file doesn't contain any rotational angular momentum G/(mR^2)\n");
        return 0;
    }

    /*--------------------------------------------------------------------------*/
    /* check if the file contains rotational angular momentum for the body.   */
    /*--------------------------------------------------------------------------*/
    if (p_pbinfile->inforec.coeffPtr[id][1] == 0)
    {
        fatalerror("Rotational angular momentum G/(mR^2) for the target object %d is not available in the "
                   "ephemeris file.\n", Target);
        return 0;
    }

    /*--------------------------------------------------------------------------*/
    /* Determine if a new record needs to be input.                             */
    /*--------------------------------------------------------------------------*/

    localmemarrcoeff = p_pbinfile->coefftime_array;
    if (Time < p_pbinfile->coefftime_array.T_beg || Time > p_pbinfile->coefftime_array.T_end)
    {
        if (!calceph_inpop_seekreadcoeff(&localmemarrcoeff, Time))
            return 0;
    }

    /*--------------------------------------------------------------------------*/
    /* Read the coefficients from the binary record.                            */
    /*--------------------------------------------------------------------------*/

    C = p_pbinfile->inforec.coeffPtr[id][0] - 1;
    N = p_pbinfile->inforec.coeffPtr[id][1];
    G = p_pbinfile->inforec.coeffPtr[id][2];
    ncomp = localmemarrcoeff.ncomp;

    calceph_interpol_PV_an(&localmemarrcoeff, TimeJD0, Timediff, Planet, C, G, N, ncomp);
    *ephunit = CALCEPH_UNIT_DAY;

    /* update the global data structure only if no prefetch */
    if (p_pbinfile->coefftime_array.prefetch == 0)
    {
        p_pbinfile->coefftime_array = localmemarrcoeff;
    }
    return 1;
}

/*--------------------------------------------------------------------------*/
/*!
     This function converts the time of the rotational angular momentum,
     which depends on time in the state vector
     according to the units in input and output.
       to the OutputUnit as output.

  it returns 0 if the function fails to convert.

  @param InputUnit  (in) output unit of the statetype
  @param OutputUnit  (in) output unit of the statetype
  @param Planet (inout) rotational angular momentum G/(mR^2)
          Input : expressed in InputUnit units.
          these units are in CALCEPH_UNIT_SEC**-1 or CALCEPH_UNIT_DAY**-1
          Output : expressed in OutputUnit units.
*/
/*--------------------------------------------------------------------------*/
static int calceph_unit_convert_rotangmom_time(stateType * Planet, int InputUnit, int OutputUnit)
{
    int res = 1;

    /* some checks */
    if ((OutputUnit & (CALCEPH_UNIT_DAY + CALCEPH_UNIT_SEC)) == 0)
    {
        res = 0;
        fatalerror("Units must include CALCEPH_UNIT_DAY or CALCEPH_UNIT_SEC \n");
    }
    else if ((OutputUnit & (CALCEPH_UNIT_DAY + CALCEPH_UNIT_SEC)) == CALCEPH_UNIT_DAY + CALCEPH_UNIT_SEC)
    {
        res = 0;
        fatalerror("Units must include CALCEPH_UNIT_DAY or CALCEPH_UNIT_SEC \n");
    }

    /* performs the conversion */
    if ((OutputUnit & CALCEPH_UNIT_DAY) == CALCEPH_UNIT_DAY && (InputUnit & CALCEPH_UNIT_SEC) == CALCEPH_UNIT_SEC)
    {
        calceph_stateType_mul_scale(Planet, 86400E0);
        calceph_stateType_mul_time(Planet, 86400E0);
    }
    if ((InputUnit & CALCEPH_UNIT_DAY) == CALCEPH_UNIT_DAY && (OutputUnit & CALCEPH_UNIT_SEC) == CALCEPH_UNIT_SEC)
    {
        calceph_stateType_div_scale(Planet, 86400E0);
        calceph_stateType_div_time(Planet, 86400E0);
    }

    return res;
}

/*--------------------------------------------------------------------------*/
/*! Return the rotational angular momentum G/(mR^2) of the object
    for a given target at the specified time
  (time elapsed from JD0).
  The output is expressed according to unit.
  The target  is expressed in the old or NAIF ID numbering system defined on unit.

  return 0 on error.
  return 1 on success.

   @param pephbin (inout) ephemeris file descriptor
   @param JD0 (in) reference time (could be 0)
   @param time (in) time elapsed from JD0
   @param target (in) "target" object (NAIF or old numbering system)
   @param unit (in) sum of CALCEPH_UNIT_??? and /or CALCEPH_USE_NAIFID
   If unit& CALCEPH_USE_NAIFID !=0 then NAIF identification numbers are used
   If unit& CALCEPH_USE_NAIFID ==0 then old identification numbers are used
   @param order (in) order of the computation
   =0 : G/(mR^2) are  computed
   =1 : G/(mR^2)+1st derivatives  are computed
   =2 : G/(mR^2)+1st and 2nd derivatives  are computed
   =3 :  G/(mR^2)+1st, 2nd and 3rd derivatives  are computed
   @param PV (out) quantities of the "target" object
     PV[0:3*(order+1)-1] are valid
*/
/*--------------------------------------------------------------------------*/
int calceph_inpop_rotangmom_unit(t_calcephbin * pephbin, double JD0, double time, int target, int unit,
                                 int order, double PV[ /*<=12 */ ])
{
    int loccoeff = -1;

    int ephunit;

    int res;

    stateType GmR2;

    GmR2.order = order;

    if ((unit & CALCEPH_USE_NAIFID) != 0)
    {
        switch (target)
        {
            case NAIFID_SUN:
                loccoeff = 10;
                break;
            case NAIFID_MERCURY:
                loccoeff = 0;
                break;
            case NAIFID_VENUS:
                loccoeff = 1;
                break;
            case NAIFID_EARTH:
                loccoeff = 2;
                break;
            case NAIFID_MARS:
                loccoeff = 3;
                break;
            case NAIFID_JUPITER:
                loccoeff = 4;
                break;
            case NAIFID_SATURN:
                loccoeff = 5;
                break;
            case NAIFID_URANUS:
                loccoeff = 6;
                break;
            case NAIFID_NEPTUNE:
                loccoeff = 7;
                break;
            case NAIFID_PLUTO:
                loccoeff = 8;
                break;
            case NAIFID_MOON:
                loccoeff = 9;
                break;

            default:           /* unsupported case */
                break;
        }
    }
    else
    {
        if (target >= 1 && target <= 11)
        {
            loccoeff = target - 1;
        }
    }

    if (loccoeff < 0)
    {
        fatalerror("Rotational angular momentum G/(mR^2) for the target object %d is not supported.\n", target);
        return 0;
    }

    res = calceph_interpol_PV_pam(&pephbin->data.binary.pam, JD0, time, target, &ephunit, loccoeff, &GmR2);
    if (res)
    {
        res = calceph_unit_convert_rotangmom_time(&GmR2, ephunit, unit);
        calceph_PV_set_stateType(PV, GmR2);
    }
    return res;
}
