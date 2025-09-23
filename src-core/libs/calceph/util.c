/*-----------------------------------------------------------------*/
/*! 
  \file util.c 
  \brief supplementary tools.
  
   Implemenation of :
   - swap byte order

  \author  M. Gastineau 
           Astronomie et Systemes Dynamiques, LTE, CNRS, Observatoire de Paris. 

   Copyright, 2008-2025, CNRS
   email of the author : Mickael.Gastineau@obspm.fr
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

/*! \def DECLARE_GLOBALVARUTIL
  global variable is declared in this file.
*/
#define DECLARE_GLOBALVARUTIL
//#if HAVE_CONFIG_H
#include "calcephconfig.h"
//#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#if defined(HAVE_MATHIMF_H)
#include <mathimf.h>
#elif defined(HAVE_MATH_H)
#include <math.h>
#endif
#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif
#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if defined(HAVE_TIME_H)
#include <time.h>
#endif
#ifdef STDC_HEADERS
#include <stdarg.h>
#endif
#include "util.h"
#include "real.h"
#include "calceph.h"
#include "calcephinternal.h"

/*--------------------------------------------------------------------------*/
/*! swap the byte order of the integer 
  @param x (in) integer
*/
/*--------------------------------------------------------------------------*/
int swapint(int x)
{
    int j;
    int res;
    char *src = (char *) &x;
    char *dst = (char *) &res;

    for (j = 0; j < (int) sizeof(int); j++)
    {
        dst[sizeof(int) - j - 1] = src[j];
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! swap the byte order of the real 
  @param x (in) real
*/
/*--------------------------------------------------------------------------*/
double swapdbl(double x)
{
    double res;
    int j;
    char *src = (char *) &x;
    char *dst = (char *) &res;

    for (j = 0; j < (int) sizeof(double); j++)
    {
        dst[sizeof(double) - j - 1] = src[j];
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! swap the byte order of the arrays of reals 
  @param x (inout) array of reals
  @param count (in) number of elements in the array
*/
/*--------------------------------------------------------------------------*/
void swapdblarray(double *x, size_t count)
{
    size_t j;

    for (j = 0; j < count; j++)
    {
        x[j] = swapdbl(x[j]);
    }
}

/*--------------------------------------------------------------------------*/
/*! swap the byte order of the arrays of integers 
  @param x (inout) array of integers
  @param count (in) number of elements in the array
*/
/*--------------------------------------------------------------------------*/
void swapintarray(int *x, int count)
{
    int j;

    for (j = 0; j < count; j++)
    {
        x[j] = swapint(x[j]);
    }
}

/*------------------------------------------------------------------*/
/*!fonction rint
  @param x (in) reel
*/
/*------------------------------------------------------------------*/
#if !HAVE_RINT
double rint(double x)
{
    double a = floor(x);
    double b = ceil(x);

    return ((fabs(x - a) < fabs(x - b)) ? a : b);
}
#endif

/*------------------------------------------------------------------*/
/* vasprintf                                                        */
/*------------------------------------------------------------------*/
#if !defined(HAVE_VASPRINTF)
int vasprintf(char **retp, const char *format, va_list args)
{
#if defined(HAVE__VSCPRINTF)
    /* code for windows for vasprintf */
    int len;
    int res;
    char *buffer;

    len = _vscprintf(format, args) + 1; /* _vscprintf doesn't count terminating '\0' */
    buffer = (char *) malloc(len * sizeof(char));
    if (buffer != NULL)
    {
        res = vsprintf(buffer, format, args);
        *retp = buffer;
    }
    else
    {
        res = -1;
        *retp = NULL;
    }
    return res;
#elif HAVE_VSNPRINTF
    char buf1[1];
    int nrequired, oldnrequired;
    char *oldretp;

    *retp = NULL;

    nrequired = vsnprintf(buf1, 1, format, args);

    while (1)
    {
        oldnrequired = nrequired;
        if (*retp != NULL)
            free(*retp);
        *retp = NULL;
        if ((*retp = (char *) malloc(nrequired + 1)) == NULL)
        {
            return -1;
        }
        nrequired = vsnprintf(*retp, nrequired + 1, format, args);
        if (nrequired == oldnrequired)
            break;
    }
    return nrequired;
#else
#error  operating system  does not support vasprintf or vsnprintf
#endif
}
#endif

/*------------------------------------------------------------------*/
/* snprintf                                                        */
/*------------------------------------------------------------------*/
#if !defined(HAVE_SNPRINTF)
int calceph_snprintf(char * str, size_t PARAMETER_UNUSED(size), const char * format, ...)
{
#if HAVE_PRAGMA_UNUSED
#pragma unused(size)
#endif
    int ret = 0;
    va_list args;
    va_start(args, format);
    ret = vsprintf(str, format, args);
    va_end(args);
    return ret;
}
#endif

/*------------------------------------------------------------------*/
/* calceph_strerror_errno : 
 return a thread-safe content of  calceph_strerror_errno(buffer_error) into buffer
 @param buffer (out) content of the error message
 return the pointer to buffer     
*/
/*------------------------------------------------------------------*/
const char *calceph_strerror_errno(buffer_error_t buffer)
{
#if HAVE_STRERROR_R
#if defined(__CYGWIN__)
#if defined(_GNU_SOURCE)
    typedef const char * strerror_r_t;
#else
    typedef int strerror_r_t;
#endif   
#elif defined(_GNU_SOURCE) && defined(__ANDROID_API__) && (__ANDROID_API__ >= 23)
    /* available on android NDK since API level 23 if _GNU_SOURCE is defined */
    typedef const char *strerror_r_t;
#elif !defined(__GLIBC__) || ((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE)
    typedef int strerror_r_t;
#else
    typedef const char *strerror_r_t;
#endif
    strerror_r_t code;

    buffer[0] = '\0';
    code = strerror_r(errno, buffer, BUFFERSIZE_STRERROR);
    (void) code;
#elif HAVE_STRERROR_S
    (void) strerror_s(buffer, BUFFERSIZE_STRERROR, errno);
#else
    const char *msg = strerror(errno);

    strncpy(buffer, msg, BUFFERSIZE_STRERROR - 1);
    buffer[BUFFERSIZE_STRERROR - 1] = '\0';
#endif
    return buffer;
}
