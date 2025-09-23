/*-----------------------------------------------------------------*/
/*! 
  \file calcephtime.c 
  \brief get time information on ephemeris data file 

  \author  M. Gastineau 
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de Paris. 

   Copyright,  2017-2024, CNRS
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
#include "calcephinternal.h"
#include "calcephspice.h"
#include "util.h"

/*--------------------------------------------------------------------------*/
/*! return the time scale in the ephemeris file 
   return 0 on error.
   return 1 if the quantities of all bodies are expressed in the TDB time scale. 
   return 2 if the quantities of all bodies are expressed in the TCB time scale. 
  
  @param eph (inout) ephemeris descriptor
*/
/*--------------------------------------------------------------------------*/
int calceph_gettimescale(t_calcephbin * eph)
{
    int res = 0;

    switch (eph->etype)
    {
        case CALCEPH_espice:
            res = calceph_spice_gettimescale(&eph->data.spkernel);
            break;

        case CALCEPH_ebinary:
            res = calceph_inpop_gettimescale(&eph->data.binary);
            break;

        default:
            /* GCOVR_EXCL_START */
            fatalerror("Unknown ephemeris type in calceph_gettimescale\n");
            /* GCOVR_EXCL_STOP */
            break;
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! This function returns the first and last time available in the ephemeris file associated
to eph. The Julian date for the first and last time are expressed in the time scale
returned by calceph_gettimescale().
 
   return 0 on error, otherwise non-zero value.
  
  @param eph (inout) ephemeris descriptor
  @param firsttime (out) the Julian date of the first time available in the ephemeris file, 
    expressed in the same time scale as calceph_gettimescale.
  @param lasttime (out) the Julian date of the last time available in the ephemeris file, 
    expressed in the same time scale as calceph_gettimescale.  
  @param continuous (out) availability of the data
     1 if the quantities of all bodies are available for any time between the first and last time.
     2 if the quantities of some bodies are available on discontinuous time intervals between the first and last time.
     3 if the quantities of each body are available on a continuous time interval between the first and last time, but not available for any time between the first and last time.
*/
/*--------------------------------------------------------------------------*/
int calceph_gettimespan(t_calcephbin * eph, double *firsttime, double *lasttime, int *continuous)
{
    int res = 0;

    switch (eph->etype)
    {
        case CALCEPH_espice:
            res = calceph_spice_gettimespan(&eph->data.spkernel, firsttime, lasttime, continuous);
            break;

        case CALCEPH_ebinary:
            res = calceph_inpop_gettimespan(&eph->data.binary, firsttime, lasttime, continuous);
            break;

        default:
            /* GCOVR_EXCL_START */
            fatalerror("Unknown ephemeris type in calceph_gettimespan\n");
            /* GCOVR_EXCL_STOP */
            break;
    }
    return res;
}
