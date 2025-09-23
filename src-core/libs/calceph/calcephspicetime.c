/*-----------------------------------------------------------------*/
/*!
  \file calcephspicetime.c
  \brief get time information on SPICE KERNEL ephemeris data file

  \author  M. Gastineau
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de
  Paris.

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
#include "util.h"

/*--------------------------------------------------------------------------*/
/*! return the time scale in the ephemeris file :
   return 0 on error.
   return 1 if the quantities of all bodies are expressed in the TDB time scale.
   return 2 if the quantities of all bodies are expressed in the TCB time scale.

  @param eph (in) ephemeris descriptor
*/
/*--------------------------------------------------------------------------*/
int calceph_spice_gettimescale(const struct calcephbin_spice *eph)
{
    int res = 0;

    struct SPICEkernel *pkernel;

    struct SPKfile *pspk;

    struct SPKSegmentList *listseg;

    int j;

    pkernel = eph->list;
    while (pkernel != NULL)
    {
        switch (pkernel->filetype)
        {
            case DAF_SPK:
            case DAF_PCK:
                pspk = &pkernel->filedata.spk;
                for (listseg = pspk->list_seg; listseg != NULL; listseg = listseg->next)
                {
                    for (j = 0; j < listseg->array_seg.count; j++)
                    {
                        struct SPKSegmentHeader *seg = listseg->array_seg.array + j;

                        switch (seg->datatype)
                        {
                            case SPK_SEGTYPE1:
                            case SPK_SEGTYPE3:
                            case SPK_SEGTYPE2:
                            case SPK_SEGTYPE5:
                            case SPK_SEGTYPE8:
                            case SPK_SEGTYPE9:
                            case SPK_SEGTYPE12:
                            case SPK_SEGTYPE13:
                            case SPK_SEGTYPE14:
                            case SPK_SEGTYPE17:
                            case SPK_SEGTYPE20:
                            case SPK_SEGTYPE21:
                                if (res == 0 || res == 1)
                                {
                                    res = 1;
                                }
                                else
                                {
                                    fatalerror("Mix the time scale TDB and TCB in the same kernel");
                                    res = -1;
                                }
                                break;

                            case SPK_SEGTYPE103:
                            case SPK_SEGTYPE120:
                                if (res == 0 || res == 2)
                                {
                                    res = 2;
                                }
                                else
                                {
                                    fatalerror("Mix the time scale TDB and TCB in the same kernel");
                                    res = -1;
                                }
                                break;

                            default:
                                break;
                        }
                    }
                }
                break;

            default:
                break;
        }
        pkernel = pkernel->next;
    }

    if (res == -1)
        res = 0;
    return res;
}

/*--------------------------------------------------------------------------*/
/*! This function returns the first and last time available in the ephemeris
file associated to eph. The Julian date for the first and last time are
expressed in the time scale returned by calceph_gettimescale().

   return 0 on error, otherwise non-zero value.

  @param eph (in) ephemeris descriptor
  @param firsttime (out) the Julian date of the first time available in the
ephemeris file, expressed in the same time scale as calceph_gettimescale.
  @param lasttime (out) the Julian date of the last time available in the
ephemeris file, expressed in the same time scale as calceph_gettimescale.
  @param continuous (out) availability of the data
     1 if the quantities of all bodies are available for any time between the
first and last time. 2 if the quantities of some bodies are available on
discontinuous time intervals between the first and last time. 3 if the
quantities of each body are available on a continuous time interval between the
first and last time, but not available for any time between the first and last
time.
*/
/*--------------------------------------------------------------------------*/
int calceph_spice_gettimespan(const struct calcephbin_spice *eph, double *firsttime, double *lasttime, int *continuous)
{
    int j;
    int foundfirsttime = 0;     /* =1 after the first found record with a time information */
    struct SPICEkernel *pkernel;
    struct SPKfile *pspk;
    struct SPKSegmentList *listseg;
    struct listobject
    {
        int target;             /* target object */
        int center;             /* center object */
        double firsttime;       /* firsttime */
        double lasttime;        /* lasttime */
        struct listobject *next;    /* next element in the list */
    } *uniqlistobject = NULL;
    struct listobject *ptr;

    *firsttime = 0;
    *lasttime = -1;
    *continuous = -1;

    pkernel = eph->list;
    while (pkernel != NULL)
    {
        switch (pkernel->filetype)
        {
            case DAF_PCK:
            case DAF_SPK:
                pspk = &pkernel->filedata.spk;
                /*
                   - create a list of objects : unique center->target
                   - add the beginning and final time for each object and set
                   continous or not
                   - check if all objects have the same time span
                 */
                for (listseg = pspk->list_seg; listseg != NULL; listseg = listseg->next)
                {
                    for (j = 0; j < listseg->array_seg.count; j++)
                    {
                        struct SPKSegmentHeader *seg = listseg->array_seg.array + j;
                        double segfirsttime = seg->T_begin / 86400E0 + 2.451545E+06;
                        double seglasttime = seg->T_end / 86400E0 + 2.451545E+06;

                        if (foundfirsttime == 0)
                        {
                            *firsttime = segfirsttime;
                            *lasttime = seglasttime;
                            *continuous = 1;
                            foundfirsttime = 1;
                        }
                        else
                        {
                            if (segfirsttime < *firsttime)
                            {
                                *firsttime = segfirsttime;
                            }
                            if (*lasttime < seglasttime)
                            {
                                *lasttime = seglasttime;
                            }
                        }
                        /* look in the list */
                        for (ptr = uniqlistobject; ptr != NULL; ptr = ptr->next)
                        {
                            if (ptr->target == seg->body && ptr->center == seg->center)
                            {
                                if (seglasttime < ptr->firsttime || ptr->lasttime < segfirsttime)
                                {   /* not continuous segment */
                                    *continuous = 2;
                                }
                                if (segfirsttime < ptr->firsttime)
                                {
                                    ptr->firsttime = segfirsttime;
                                }
                                if (ptr->lasttime < seglasttime)
                                {
                                    ptr->lasttime = seglasttime;
                                }
                                break;
                            }
                        }
                        /* if not found , create a new element at the head */
                        if (ptr == NULL)
                        {
                            ptr = (struct listobject *) malloc(sizeof(struct listobject));
                            if (ptr == NULL)
                            {
                                /* GCOVR_EXCL_START */
                                fatalerror("Can't allocate memory for %lu bytes.\n",
                                           (unsigned long) sizeof(struct listobject));
                                return 0;
                                /* GCOVR_EXCL_STOP */
                            }
                            ptr->target = seg->body;
                            ptr->center = seg->center;
                            ptr->firsttime = segfirsttime;
                            ptr->lasttime = seglasttime;
                            ptr->next = uniqlistobject;
                            uniqlistobject = ptr;
                        }
                    }
                }
                break;

            case TXT_PCK:
            case TXT_FK:
                break;

            default:
                fatalerror("Unknown SPICE type in %d\n", (int) pkernel->filetype);
                break;
        }
        pkernel = pkernel->next;
    }

    /* check if all objects have the same time span */
    if (*continuous != 2)
    {
        for (ptr = uniqlistobject; ptr != NULL; ptr = ptr->next)
        {
            if (ptr->firsttime != *firsttime || ptr->lasttime != *lasttime)
            {
                *continuous = 3;
            }
        }
    }
    /* free the list */
    for (ptr = uniqlistobject; ptr != NULL;)
    {
        uniqlistobject = ptr->next;
        free(ptr);
        ptr = uniqlistobject;
    }

    if (foundfirsttime == 0)
    {
        fatalerror("The SPICE kernels do not contain any segment with a time span.\n");
    }
    return (foundfirsttime != 0);
}
