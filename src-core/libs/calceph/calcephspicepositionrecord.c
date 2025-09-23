/*-----------------------------------------------------------------*/
/*!
  \file calcephspicepositionrecord.c
  \brief get position’s records information on SPICE KERNEL ephemeris data file

  \author  M. Gastineau
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de Paris.

   Copyright, 2017, 2018, 2019, 2020, 2021, CNRS
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
#if HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include "calcephdebug.h"
#include "real.h"
#define __CALCEPH_WITHIN_CALCEPH 1
#include "calceph.h"
#include "calcephinternal.h"
#include "util.h"
#include "calcephchebyshev.h"

/*--------------------------------------------------------------------------*/
/*! return the number of position’s records available in the ephemeris file

  @param eph (in) ephemeris descriptor
*/
/*--------------------------------------------------------------------------*/
int calceph_spice_getpositionrecordcount(const struct calcephbin_spice *eph)
{
    int res = 0;

    struct SPICEkernel *pkernel;

    struct SPKfile *pspk;

    struct SPKSegmentList *listseg;

    pkernel = eph->list;
    while (pkernel != NULL)
    {
        switch (pkernel->filetype)
        {
            case DAF_SPK:
                pspk = &pkernel->filedata.spk;
                for (listseg = pspk->list_seg; listseg != NULL; listseg = listseg->next)
                {
                    res += listseg->array_seg.count;
                }
                break;

            case DAF_PCK:
            case TXT_PCK:
            case TXT_FK:
                break;

            default:
                fatalerror("Unknown SPICE type in %d\n", (int) pkernel->filetype);
                break;
        }
        pkernel = pkernel->next;
    }

    return res;
}

/*--------------------------------------------------------------------------*/
/*! This function returns the target and origin bodies, the first and last time,
 and the reference frame available at the specified index
 for the position's records of the ephemeris file  associated to eph.

   return 0 on error, otherwise non-zero value.

  @param eph (in) ephemeris descriptor
  @param index (in) index of the position’s record, between 1 and calceph_getpositionrecordcount()
  @param target – The target body
  @param center – The origin body
  @param firsttime (out) the Julian date of the first time available in this record,
    expressed in the same time scale as calceph_gettimescale.
  @param lasttime (out) the Julian date of the first time available in this record,
    expressed in the same time scale as calceph_gettimescale.
  @param frame (out) reference frame
  1  = ICRF
  @param segid (out) type of segment
*/
/*--------------------------------------------------------------------------*/
int calceph_spice_getpositionrecordindex(const struct calcephbin_spice *eph, int index, int *target,
                                         int *center, double *firsttime, double *lasttime, int *frame, int *segid)
{
    int j;

    struct SPICEkernel *pkernel;

    struct SPKfile *pspk;

    struct SPKSegmentList *listseg;

    pkernel = eph->list;
    while (pkernel != NULL)
    {
        switch (pkernel->filetype)
        {
            case DAF_SPK:
                pspk = &pkernel->filedata.spk;
                for (listseg = pspk->list_seg; listseg != NULL; listseg = listseg->next)
                {
                    for (j = 0; j < listseg->array_seg.count; j++)
                    {
                        index--;
                        if (index == 0)
                        {
                            struct SPKSegmentHeader *seg = listseg->array_seg.array + j;

                            *firsttime = seg->T_begin / 86400E0 + 2.451545E+06;
                            *lasttime = seg->T_end / 86400E0 + 2.451545E+06;
                            *target = seg->body;
                            *center = seg->center;
                            *frame = seg->refframe;
                            *segid = seg->datatype;
                            return 1;
                        }
                    }
                }
                break;

            case DAF_PCK:
            case TXT_PCK:
            case TXT_FK:
                break;

            default:
                fatalerror("Unknown SPICE type in %d\n", (int) pkernel->filetype);
                break;
        }
        pkernel = pkernel->next;
    }
    return 0;
}
