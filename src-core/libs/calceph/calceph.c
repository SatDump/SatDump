/*-----------------------------------------------------------------*/
/*!
  \file calceph.c
  \brief high-level API
         access and interpolate INPOP and JPL Ephemeris data.
         compute position and velocity from binary ephemeris file.

  \author  M. Gastineau
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de Paris.

   Copyright, 2008-2024, CNRS
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

#include "calcephconfig.h"
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_LIMITS_H
#include <limits.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif                          /*HAVE_PTHREAD */
#include <stdlib.h>
#ifdef STDC_HEADERS
#include <stdarg.h>
#endif
#define __CALCEPH_WITHIN_CALCEPH 1
#include "calceph.h"
#include "real.h"
#include "calcephinternal.h"
#include "util.h"

/********************************************************/
/* local variables */
/********************************************************/
#ifdef HAVE_PTHREAD
/* version thread-safe */

/*! key to access pephbin structure for thread-safe */
static pthread_key_t calceph_key;

/*! key to run only one time calcephinit_once (thread-safe) */
static pthread_once_t calceph_key_once = PTHREAD_ONCE_INIT;

static void calceph_key_init(void);

#else                           /*HAVE_PTHREAD */

/* version not thread-safe */
/*! description of the ephemeris file  */
static t_calcephbin *pephbin = NULL;
#endif                          /*HAVE_PTHREAD */

/*! error variable */
struct st_calceph_sglobal
{
    union ufuncerror
    {
        void (*cusererror)(const char *);   /*!< user function callback error for C or F2003 */
        void (*f90usererror)(const char *, int len);    /*!< user function callback error for F90 */
    } func;
    int usertypeerror;          /*!< error type
                                   1 : prints the message and the program continues (default)
                                   2 : prints the message and the program exits (call exit(1))
                                   3 : call the C or F2003 function userfunc and the program continues
                                   -3 : call the F90 function userfunc and the program continues */
} calceph_sglobal = { {NULL}, 1 };

/********************************************************/
/*! Open the specified ephemeris file
  return 0 on error.
  return 1 on success.

 @param filename (in) nom du fichier
*/
/********************************************************/
int calceph_sopen(const char *filename)
{
#ifdef HAVE_PTHREAD
    /*thread-safe */
    t_calcephbin *pephbin = NULL;

    pthread_once(&calceph_key_once, calceph_key_init);

    pephbin = calceph_open(filename);

    if (pthread_setspecific(calceph_key, pephbin) != 0)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't specify thread-data for calceph_sopen.\n");
        /* GCOVR_EXCL_STOP */
    }
#else                           /*HAVE_PTHREAD */
    pephbin = calceph_open(filename);
#endif                          /*HAVE_PTHREAD */
    return (pephbin != NULL);
}

/********************************************************/
/*! Close the  ephemeris file
 */
/********************************************************/
void calceph_sclose(void)
{
#ifdef HAVE_PTHREAD
    /*thread-safe */
    t_calcephbin *pephbin;

    pephbin = (t_calcephbin *) pthread_getspecific(calceph_key);
#endif                          /*HAVE_PTHREAD */
    if (pephbin == NULL)
    {                           /*first call => initialize for this thread */
        fatalerror("calceph_sopen must be called before calceph_sclose (for each thread).\n");
    }
    else
    {
        calceph_close(pephbin);
    }
}

/********************************************************/
/*! Return the position and the velocity of the object
  "target" in the vector PV at the specified time
  (time elapsed from JD0).
  The center is specified in center.

  return 0 on error.
  return 1 on success.

   @param JD0 (in) reference time (could be 0)
   @param time (in) time elapsed from JD0
   @param target (in) "target" object
   @param center (in) "center" object
   @param PV (out) position and the velocity of the "target" object
*/
/********************************************************/
int calceph_scompute(treal JD0, treal time, int target, int center, treal PV[6])
{
    int res = 0;

#ifdef HAVE_PTHREAD
    /*thread-safe */
    t_calcephbin *pephbin;

    pephbin = (t_calcephbin *) pthread_getspecific(calceph_key);
#endif                          /*HAVE_PTHREAD */
    if (pephbin == NULL)
    {                           /*first call => initialize for this thread */
        fatalerror("calceph_sopen must be called before calceph_scompute (for each thread).\n");
    }
    else
    {
        res = calceph_compute(pephbin, JD0, time, target, center, PV);
    }
    return res;
}

/********************************************************/
/*! return the first value of the specified constant from the ephemeris file
  return 0 if constant is not found
  return the number of values associated to the constant and set value

  @param name (in) constant name
  @param value (out) value of the constant
*/
/********************************************************/
int calceph_sgetconstant(const char *name, double *value)
{
    int res = 0;

#ifdef HAVE_PTHREAD
    /*thread-safe */
    t_calcephbin *pephbin;

    pephbin = (t_calcephbin *) pthread_getspecific(calceph_key);
#endif                          /*HAVE_PTHREAD */
    if (pephbin == NULL)
    {                           /*first call => initialize for this thread */
        fatalerror("calceph_sopen must be called before calceph_sgetconstant (for each thread).\n");
    }
    else
    {
        res = calceph_getconstant(pephbin, name, value);
    }
    return res;
}

/********************************************************/
/*! return the number of constants available in the ephemeris file

*/
/********************************************************/
int calceph_sgetconstantcount(void)
{
    int res = 0;

#ifdef HAVE_PTHREAD
    /*thread-safe */
    t_calcephbin *pephbin;

    pephbin = (t_calcephbin *) pthread_getspecific(calceph_key);
#endif                          /*HAVE_PTHREAD */
    if (pephbin == NULL)
    {                           /*first call => initialize for this thread */
        fatalerror("calceph_sopen must be called before calceph_sgetconstantcount (for each thread).\n");
    }
    else
    {
        res = calceph_getconstantcount(pephbin);
    }
    return res;
}

/********************************************************/
/*! get the name and its value of the constant available
    at some index from the ephemeris file
   return the number of values associated to the constant and set value
   return 0 if index isn't valid

  @param index (in) index of the constant (must be between 1 and calceph_getconstantcount() )
  @param name (out) name of the constant
  @param value (out) first value of the constant
*/
/********************************************************/
int calceph_sgetconstantindex(int index, char name[CALCEPH_MAX_CONSTANTNAME], double *value)
{
    int res = 0;

#ifdef HAVE_PTHREAD
    /*thread-safe */
    t_calcephbin *pephbin;

    pephbin = (t_calcephbin *) pthread_getspecific(calceph_key);
#endif                          /*HAVE_PTHREAD */
    if (pephbin == NULL)
    {                           /*first call => initialize for this thread */
        fatalerror("calceph_sopen must be called before calceph_sgetconstantindex (for each thread).\n");
    }
    else
    {
        res = calceph_getconstantindex(pephbin, index, name, value);
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! return the time scale in the ephemeris file
   return 0 on error.
   return 1 if the quantities of all bodies are expressed in the TDB time scale.
   return 2 if the quantities of all bodies are expressed in the TCB time scale.
*/
/*--------------------------------------------------------------------------*/
int calceph_sgettimescale(void)
{
    int res = 0;

#ifdef HAVE_PTHREAD
    /*thread-safe */
    t_calcephbin *pephbin;

    pephbin = (t_calcephbin *) pthread_getspecific(calceph_key);
#endif                          /*HAVE_PTHREAD */
    if (pephbin == NULL)
    {                           /*first call => initialize for this thread */
        fatalerror("calceph_sopen must be called before calceph_sgettimescale (for each thread).\n");
    }
    else
    {
        res = calceph_gettimescale(pephbin);
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! This function returns the first and last time available.
The Julian date for the first and last time are expressed in the time scale
returned by calceph_sgettimescale().

   return 0 on error, otherwise non-zero value.

  @param firsttime (out) the Julian date of the first time available in the ephemeris file,
    expressed in the same time scale as calceph_gettimescale.
  @param lasttime (out) the Julian date of the last time available in the ephemeris file,
    expressed in the same time scale as calceph_gettimescale.
  @param continuous (out) availability of the data
     1 if the quantities of all bodies are available for any time between the first and last time.
     2 if the quantities of some bodies are available on discontinuous time intervals between the first
and last time. 3 if the quantities of each body are available on a continuous time interval between the
first and last time, but not available for any time between the first and last time.
*/
/*--------------------------------------------------------------------------*/
int calceph_sgettimespan(double *firsttime, double *lasttime, int *continuous)
{
    int res = 0;

#ifdef HAVE_PTHREAD
    /*thread-safe */
    t_calcephbin *pephbin;

    pephbin = (t_calcephbin *) pthread_getspecific(calceph_key);
#endif                          /*HAVE_PTHREAD */
    if (pephbin == NULL)
    {                           /*first call => initialize for this thread */
        fatalerror("calceph_sopen must be called before calceph_sgettimespan (for each thread).\n");
    }
    else
    {
        res = calceph_gettimespan(pephbin, firsttime, lasttime, continuous);
    }
    return res;
}

/********************************************************/
/*! store in szversion the version of the ephemeris file
    as a null-terminated string

  return 0 on error.
  return 1 on success.

 @param szversion (out) null-terminated string of the version of the ephemeris file
*/
/********************************************************/
int calceph_sgetfileversion(char szversion[CALCEPH_MAX_CONSTANTVALUE])
{
    int res = 0;

#ifdef HAVE_PTHREAD
    /*thread-safe */
    t_calcephbin *pephbin;

    pephbin = (t_calcephbin *) pthread_getspecific(calceph_key);
#endif                          /*HAVE_PTHREAD */
    if (pephbin == NULL)
    {                           /*first call => initialize for this thread */
        fatalerror("calceph_sopen must be called before calceph_sgetfileversion (for each thread).\n");
    }
    else
    {
        res = calceph_getfileversion(pephbin, szversion);
    }
    return res;
}

/********************************************************/
/*! Return the position and the velocity of the object
  "target" in the vector PV at the specified time
  (time elapsed from JD0).
  The center is specified in center.

  return 0 on error.
  return 1 on success.

   @param pephbin (inout) ephemeris file descriptor
   @param JD0 (in) reference time (could be 0)
   @param time (in) time elapsed from JD0
   @param target (in) "target" object
   @param center (in) "center" object
   @param PV (out) position and the velocity of the "target" object
*/
/********************************************************/
int calceph_compute(t_calcephbin * pephbin, treal JD0, treal time, int target, int center, treal PV[6])
{
    int unit = CALCEPH_UNIT_AU + CALCEPH_UNIT_DAY + CALCEPH_UNIT_RAD;

    if (target == TTMTDB + 1 || target == TCGMTCB + 1)
        unit = CALCEPH_UNIT_SEC;
    return calceph_compute_unit(pephbin, JD0, time, target, center, unit, PV);
}

/********************************************************/
/*!  set the error handler for the calceph library

  @param type (in) possible value :
             1 : prints the message and the program continues (default)
             2 : prints the message and the program exits (call exit(1))
             3 : call the function C or F2003 userfunc and the programm continues
  @param userfunc (in) pointer to a function
             useful only if type = 3
             This function is called on error with a message as the first argument
*/
/********************************************************/
void calceph_seterrorhandler(int type, void (*userfunc)(const char *))
{
    if (type >= 1 && type <= 3)
    {
        calceph_sglobal.func.cusererror = userfunc;
        calceph_sglobal.usertypeerror = type;
    }
}

/********************************************************/
/*!  function called on error.
  depending on the value of calceph_sglobal.usertypeerror,
             1 : prints the message and the program continues (default)
             2 : prints the message and the program exits (call exit(1))
             3 : call the function userfunc and the programm continues

  @param format (in) format of the message (like printf)
*/
/********************************************************/
void calceph_fatalerror(const char *format, ...)
{
    typedef void (*t_f90userfuncerror)(const char *, int len);  /*!< user function callback  */

    char *msg = NULL;

    const char *defaultmsg = "Not enough memory";

    t_f90userfuncerror ptrfunc = (t_f90userfuncerror) calceph_sglobal.func.f90usererror;

    va_list args;

    va_start(args, format);

    switch (calceph_sglobal.usertypeerror)
    {
        case 3:
            if (vasprintf(&msg, format, args) >= 0)
            {
                ptrfunc(msg, (int) strlen(msg));    /* give length for F90 functions */
                free(msg);
            }
            else
            {
                ptrfunc(defaultmsg, (int) strlen(defaultmsg));
            }
            break;

        case 2:
            printf("CALCEPH ERROR : ");
            vprintf(format, args);
            exit(1);
            break;

        case 1:
        default:
            printf("CALCEPH ERROR : ");
            vprintf(format, args);
            break;
    }
    va_end(args);
}

#ifdef HAVE_PTHREAD
/********************************************************/
/*!
    Initialize calceph_key for thread-safe usage\n
    Must be called only by fcntabtcheb (once time )
*/
/********************************************************/
static void calceph_key_init(void)
{
    if (pthread_key_create(&calceph_key, NULL) != 0)
        fatalerror("Can't initialize thread-key for calceph_init.\n");
}
#endif                          /*HAVE_PTHREAD */
