/*-----------------------------------------------------------------*/
/*!
  \file interfaces_C_F90.c
  \brief Fortran 77/90/95 interface for Calceph.

  \author  M. Gastineau, H. Manche
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de
  Paris.

   Copyright, 2008-2024,CNRS
   email of the author : Mickael.Gastineau@obspm.fr, Herve.Manche@obspm.fr

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
#include "real.h"
#include "util.h"
#define __CALCEPH_WITHIN_CALCEPH 1
#include "calceph.h"
#include "calcephinternal.h"

#if __STRICT_ANSI__
#define inline
#endif

/* if the fortran is disabled */
#ifndef FC_FUNC_
#define FC_FUNC_(x, y) x
#endif

int FC_FUNC_(f90calceph_sopen, F90CALCEPH_SOPEN) (char *filename, long int len);

void FC_FUNC_(f90calceph_sclose, F90CALCEPH_SCLOSE) (void);

int FC_FUNC_(f90calceph_scompute, F90CALCEPH_SCOMPUTE) (double *JD0, double *time, int *target,
                                                        int *center, double PV[6]);
int FC_FUNC_(f90calceph_sgetconstant, F90CALCEPH_SGETCONSTANT) (char *name, double *value, long int len);

int FC_FUNC_(f90calceph_sgetconstantcount, F90CALCEPH_SGETCONSTANTCOUNT) (void);

int FC_FUNC_(f90calceph_sgetconstantindex,
             F90CALCEPH_SGETCONSTANTINDEX) (int *index, char name[CALCEPH_MAX_CONSTANTNAME], double *value);
int FC_FUNC_(f90calceph_sgettimescale, F90CALCEPH_SGETTIMESCALE) (void);
int FC_FUNC_(f90calceph_sgettimespan, F90CALCEPH_SGETTIMESPAN) (double *firsttime, double *lasttime, int *continuous);
int FC_FUNC_(f90calceph_sgetfileversion, F90CALCEPH_SGETFILEVERSION) (char version[CALCEPH_MAX_CONSTANTVALUE]);

int FC_FUNC_(f90calceph_open, F90CALCEPH_OPEN) (long long *eph, char *filename, long int len);

int FC_FUNC_(f90calceph_open_array, F90CALCEPH_OPEN_ARRAY) (long long *eph, int *n, char *filename, int *len);

void FC_FUNC_(f90calceph_close, F90CALCEPH_CLOSE) (long long *eph);

int FC_FUNC_(f90calceph_prefetch, F90CALCEPH_PREFETCH) (long long *eph);

int FC_FUNC_(f90calceph_isthreadsafe, F90CALCEPH_ISTHREADSAFE) (long long *eph);

void FC_FUNC_(f90calceph_seterrorhandler, F90CALCEPH_SETERRORHANDLER) (int *type, void (*userfunc) (const char *));

int FC_FUNC_(f90calceph_getconstantindex,
             F90CALCEPH_GETCONSTANTINDEX) (long long *eph, int *index,
                                           char name[CALCEPH_MAX_CONSTANTNAME], double *value);
int FC_FUNC_(f90calceph_getconstant, F90CALCEPH_GETCONSTANT) (long long *eph, char *name, double *value, long int len);

int FC_FUNC_(f90calceph_getconstantsd, F90CALCEPH_GETCONSTANTSD) (long long *eph, char *name,
                                                                  double *value, long int len);

int FC_FUNC_(f90calceph_getconstantvd, F90CALCEPH_GETCONSTANTVD) (long long *eph, char *name,
                                                                  double *arrayvalue, const int *nvalue, long int len);

int FC_FUNC_(f90calceph_getconstantss, F90CALCEPH_GETCONSTANTSS) (long long *eph, char *name,
                                                                  t_calcephcharvalue value, long int len);

int FC_FUNC_(f90calceph_getconstantvs, F90CALCEPH_GETCONSTANTVS) (long long *eph, char *name,
                                                                  t_calcephcharvalue * arrayvalue,
                                                                  const int *nvalue, long int len);

int FC_FUNC_(f90calceph_getconstantcount, F90CALCEPH_GETCONSTANTCOUNT) (long long *eph);

int FC_FUNC_(f90calceph_gettimescale, F90CALCEPH_GETTIMESCALE) (long long *eph);
int FC_FUNC_(f90calceph_gettimespan, F90CALCEPH_GETTIMESPAN) (long long *eph, double *firsttime,
                                                              double *lasttime, int *continuous);
int FC_FUNC_(f90calceph_getidbyname, F90CALCEPH_GETIDBYNAME) (long long *eph, char *name, int *unit,
                                                              int *id, long int len);
int FC_FUNC_(f90calceph_getnamebyidss, F90CALCEPH_GETNAMEBYIDSS) (long long *eph, int *id, int *unit,
                                                                  t_calcephcharvalue value);
int FC_FUNC_(f90calceph_getfileversion,
             F90CALCEPH_GETFILEVERSION) (long long *eph, char version[CALCEPH_MAX_CONSTANTVALUE]);
int FC_FUNC_(f90calceph_getpositionrecordcount, F90CALCEPH_GETPOSITIONRECORDCOUNT) (long long *eph);
int FC_FUNC_(f90calceph_getpositionrecordindex,
             F90CALCEPH_GETPOSITIONRECORDINDEX) (long long *eph, int *index, int *target, int *center,
                                                 double *firsttime, double *lasttime, int *frame);
int FC_FUNC_(f90calceph_getpositionrecordindex2,
             F90CALCEPH_GETPOSITIONRECORDINDEX2) (long long *eph, int *index, int *target, int *center,
                                                  double *firsttime, double *lasttime, int *frame, int *segid);
int FC_FUNC_(f90calceph_getorientrecordcount, F90CALCEPH_GETORIENTRECORDCOUNT) (long long *eph);
int FC_FUNC_(f90calceph_getorientrecordindex,
             F90CALCEPH_GETORIENTRECORDINDEX) (long long *eph, int *index, int *target, double *firsttime,
                                               double *lasttime, int *frame);
int FC_FUNC_(f90calceph_getorientrecordindex2,
             F90CALCEPH_GETORIENTRECORDINDEX2) (long long *eph, int *index, int *target,
                                                double *firsttime, double *lasttime, int *frame, int *segtype);
int FC_FUNC_(f90calceph_compute, F90CALCEPH_COMPUTE) (long long *eph, double *JD0, double *time,
                                                      int *target, int *center, double PV[6]);
int FC_FUNC_(f90calceph_compute_order, F90CALCEPH_COMPUTE_ORDER) (long long *eph, double *JD0,
                                                                  double *time, int *target, int *center,
                                                                  int *unit, int *order, double *PVAJ);
int FC_FUNC_(f90calceph_orient_order, F90CALCEPH_ORIENT_ORDER) (long long *eph, double *JD0, double *time,
                                                                int *target, int *unit, int *order, double *PVAJ);
int FC_FUNC_(f90calceph_rotangmom_order,
             F90CALCEPH_ROTANGMOM_ORDER) (long long *eph, double *JD0, double *time, int *target,
                                          int *unit, int *order, double *PVAJ);
int FC_FUNC_(f90calceph_compute_unit, F90CALCEPH_COMPUTE_UNIT) (long long *eph, double *JD0, double *time,
                                                                int *target, int *center, int *unit, double PV[6]);
int FC_FUNC_(f90calceph_orient_unit, F90CALCEPH_ORIENT_UNIT) (long long *eph, double *JD0, double *time,
                                                              int *target, int *unit, double PV[6]);
int FC_FUNC_(f90calceph_rotangmom_unit, F90CALCEPH_ROTANGMOM_UNIT) (long long *eph, double *JD0,
                                                                    double *time, int *target, int *unit, double PV[6]);
int FC_FUNC_(f90calceph_getmaxsupportedorder, F90CALCEPH_GETMAXSUPPORTEDORDER) (int *order);

void *f2003calceph_open_array(int n, char *filename, int len);

int f2003calceph_sgetconstantindex(int index, char name[CALCEPH_MAX_CONSTANTNAME], double *value);

int f2003calceph_getconstantindex(t_calcephbin * eph, int index, char name[CALCEPH_MAX_CONSTANTNAME], double *value);

void FC_FUNC_(f90calceph_getversion_str, F90CALCEPH_GETVERSION_STR) (char version[CALCEPH_MAX_CONSTANTNAME]);

void f2003calceph_getversion_str(char version[CALCEPH_MAX_CONSTANTNAME]);

int f2003calceph_sgetfileversion(char version[CALCEPH_MAX_CONSTANTVALUE]);
int f2003calceph_getfileversion(t_calcephbin * eph, char version[CALCEPH_MAX_CONSTANTVALUE]);

int calceph_(double *JD0, double *time, int *target, int *center, double PV[6]);

int calcephinit_(char *filename, long int len);

static inline t_calcephbin *f90calceph_getaddresseph(long long *eph);
static inline long long f90calceph_getaddresseph_longlong(t_calcephbin * eph);
static void calceph_fortranfillspace(char *name, size_t len);

/********************************************************/
/*! Open the specified ephemeris file
  return 0 on error.
  return 1 on success.

 @param filename (in) nom du fichier
*/
/********************************************************/
int FC_FUNC_(f90calceph_sopen, F90CALCEPH_SOPEN) (char *filename, long int len)
{
    int res = 0;

    char *newname;

    newname = (char *) malloc(sizeof(char) * (len + 1));
    if (newname != NULL)
    {
        memcpy(newname, filename, len * sizeof(char));
        newname[len] = '\0';
        res = calceph_sopen(newname);
        free(newname);
    }
    /* GCOVR_EXCL_START */
    else
    {
        buffer_error_t buffer_error;

        fatalerror("Can't allocate memory for f90calceph_sopen\nSystem error : '%s'\n",
                   calceph_strerror_errno(buffer_error));
    }
    /* GCOVR_EXCL_STOP */
    return res;
}

/********************************************************/
/*! Close the  ephemeris file
 */
/********************************************************/
void FC_FUNC_(f90calceph_sclose, F90CALCEPH_SCLOSE) (void)
{
    calceph_sclose();
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
int FC_FUNC_(f90calceph_scompute, F90CALCEPH_SCOMPUTE) (double *JD0, double *time, int *target,
                                                        int *center, double PV[6])
{
    return calceph_scompute(*JD0, *time, *target, *center, PV);
}

/********************************************************/
/*! return the value of the specified constant frothe ephemeris file
  return 0 if constant is not found
  return 1 if constant is found and set value.

  @param name (in) constant name
  @param value (out) value of the constant
*/
/********************************************************/
int FC_FUNC_(f90calceph_sgetconstant, F90CALCEPH_SGETCONSTANT) (char *name, double *value, long int len)
{
    int res = 0;

    char *newname;

    newname = (char *) malloc(sizeof(char) * (len + 1));
    if (newname != NULL)
    {
        memcpy(newname, name, len * sizeof(char));
        newname[len] = '\0';
        res = calceph_sgetconstant(newname, value);
        free(newname);
    }
    /* GCOVR_EXCL_START */
    else
    {
        buffer_error_t buffer_error;

        fatalerror("Can't allocate memory for f90calceph_sgetconstant\nSystem "
                   "error : '%s'\n", calceph_strerror_errno(buffer_error));
    }
    /* GCOVR_EXCL_STOP */
    return res;
}

/********************************************************/
/*! return the number of constants available in the ephemeris file

*/
/********************************************************/
int FC_FUNC_(f90calceph_sgetconstantcount, F90CALCEPH_SGETCONSTANTCOUNT) (void)
{
    return calceph_sgetconstantcount();
}

/********************************************************/
/*! get the name and its value of the constant available
    at some index from the ephemeris file
   return 1 if index is valid
   return 0 if index isn't valid

  @param index (in) index of the constant (must be between 1 and
  calceph_getconstantcount() )
  @param name (out) name of the constant
  @param value (out) value of the constant
*/
/********************************************************/
int FC_FUNC_(f90calceph_sgetconstantindex,
             F90CALCEPH_SGETCONSTANTINDEX) (int *index, char name[CALCEPH_MAX_CONSTANTNAME], double *value)
{
    int res = calceph_sgetconstantindex(*index, name, value);

    if (res == 1)
    {
        calceph_fortranfillspace(name, CALCEPH_MAX_CONSTANTNAME);
    }
    return res;
}

/********************************************************/
/*! get the name and its value of the constant available
    at some index from the ephemeris file
   return 1 if index is valid
   return 0 if index isn't valid

  @param index (in) index of the constant (must be between 1 and
  calceph_sgetconstantcount() )
  @param name (out) name of the constant
  @param value (out) value of the constant
*/
/********************************************************/
int f2003calceph_sgetconstantindex(int index, char name[CALCEPH_MAX_CONSTANTNAME], double *value)
{
    int res = calceph_sgetconstantindex(index, name, value);

    if (res == 1)
    {
        calceph_fortranfillspace(name, CALCEPH_MAX_CONSTANTNAME);
    }
    return res;
}

/********************************************************/
/*! return the time scale used in the ephemeris file
 */
/********************************************************/
int FC_FUNC_(f90calceph_sgettimescale, F90CALCEPH_SGETTIMESCALE) (void)
{
    return calceph_sgettimescale();
}

/********************************************************/
/*! This function returns the first and last time available in the ephemeris
file. The Julian date for the first and last time are expressed in the time
scale returned by calceph_gettimescale().

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
/********************************************************/
int FC_FUNC_(f90calceph_sgettimespan, F90CALCEPH_SGETTIMESPAN) (double *firsttime, double *lasttime, int *continuous)
{
    return calceph_sgettimespan(firsttime, lasttime, continuous);
}

/********************************************************/
/*! store, in version, the file version of the ephemeris file.
   return 0 if the file version was not found.
   return 1 on sucess.

  @param version (out) fortran string of the version of the ephemeris
  file

*/
/********************************************************/
int FC_FUNC_(f90calceph_sgetfileversion, F90CALCEPH_SGETFILEVERSION) (char version[CALCEPH_MAX_CONSTANTVALUE])
{
    return f2003calceph_sgetfileversion(version);
}

/********************************************************/
/*! store, in version, the file version of the ephemeris file.
   return 0 if the file version was not found.
   return 1 on sucess.

  @param version (out) fortran string of the version of the ephemeris
  file

*/
/********************************************************/
int f2003calceph_sgetfileversion(char version[CALCEPH_MAX_CONSTANTVALUE])
{
    int res = calceph_sgetfileversion(version);

    calceph_fortranfillspace(version, CALCEPH_MAX_CONSTANTVALUE);
    return res;
}

/********************************************************/
/*! return eph as an address
  @param eph (inout) parameter to cast
*/
/********************************************************/
static inline t_calcephbin *f90calceph_getaddresseph(long long *eph)
{
    union ucasttopointer
    {
        long long ll;
        t_calcephbin *ptr;
    } a;

    a.ll = *eph;
    return a.ptr /*(t_calcephbin *) (*eph) */ ;
}

/********************************************************/
/*! return eph as an long long
  @param eph (inout) parameter to cast
*/
/********************************************************/
static inline long long f90calceph_getaddresseph_longlong(t_calcephbin * eph)
{
    union ucasttopointer
    {
        long long ll;
        t_calcephbin *ptr;
    } a;

    /* next initialization to 0 is required
       if sizeof(long long)>sizeof(_calcephbin *), e.g. 32-bit operating system */
    a.ll = 0;
    a.ptr = eph;
    return a.ll /*(long long ) (eph) */ ;
}

/********************************************************/
/*! Open the specified ephemeris file
  return 0 on error.
  return 1 on success.

 @param eph (out) ephemeris descriptor (as integer for F77 compatibility)
 @param filename (in) file name
*/
/********************************************************/
int FC_FUNC_(f90calceph_open, F90CALCEPH_OPEN) (long long *eph, char *filename, long int len)
{
    int res = 0;

    char *newname;

    newname = (char *) malloc(sizeof(char) * (len + 1));
    if (newname != NULL)
    {
        memcpy(newname, filename, len * sizeof(char));
        newname[len] = '\0';
        *eph = f90calceph_getaddresseph_longlong(calceph_open(newname));
        res = (*eph != 0);
        free(newname);
    }
    /* GCOVR_EXCL_START */
    else
    {
        buffer_error_t buffer_error;

        fatalerror("Can't allocate memory for f90calceph_open\nSystem error : '%s'\n",
                   calceph_strerror_errno(buffer_error));
    }
    /* GCOVR_EXCL_STOP */
    return res;
}

/********************************************************/
/*! Open the list of specified ephemeris file  - Fortran 2003 interface
  Use C binding interface.
  return the ephemeris descriptor.

 @param n (in)  number of file
 @param filename (in) array of file name
 @param len (in) length of each file name
*/
/********************************************************/
void *f2003calceph_open_array(int n, char *filename, int len)
{
    int k, l;

    char **filear;

    char *newname;

    void *eph = NULL;

    filear = (char **) malloc(sizeof(char *) * (n));
    newname = (char *) malloc(sizeof(char) * (len + 1) * (n));
    if (newname != NULL || filear != NULL)
    {
        for (k = 0; k < n; k++)
        {
            filear[k] = newname + (len + 1) * k;
            memcpy(filear[k], filename + len * k, len * sizeof(char));
            filear[k][len] = '\0';
            l = len - 1;
            while (l > 0 && filear[k][l] == ' ')
            {
                filear[k][l] = '\0';
                l--;
            }
        }
        eph = calceph_open_array(n, (const char *const *) filear);
    }
    /* GCOVR_EXCL_START */
    else
    {
        buffer_error_t buffer_error;

        fatalerror("Can't allocate memory for f90calceph_open\nSystem error : '%s'\n",
                   calceph_strerror_errno(buffer_error));
    }
    /* GCOVR_EXCL_STOP */
    if (newname != NULL)
        free(newname);
    if (filear != NULL)
        free(filear);
    return eph;
}

/********************************************************/
/*! Open the list of specified ephemeris file
  return 0 on error.
  return 1 on success.

 @param eph (out) ephemeris descriptor (as integer for F77 compatibility)
 @param n (in)  number of file
 @param filename (in) array of file name
 @param len (in) length of each file name
*/
/********************************************************/
int FC_FUNC_(f90calceph_open_array, F90CALCEPH_OPEN_ARRAY) (long long *eph, int *n, char *filename, int *len)
{
    int res;

    *eph = f90calceph_getaddresseph_longlong(f2003calceph_open_array(*n, filename, *len));
    res = (*eph != 0);
    return res;
}

/********************************************************/
/*! Close the  ephemeris file
  @param eph (in) ephemeris descriptor (as integer for F77 compatibility)
*/
/********************************************************/
void FC_FUNC_(f90calceph_close, F90CALCEPH_CLOSE) (long long *eph)
{
    calceph_close(f90calceph_getaddresseph(eph));
}

/********************************************************/
/*! Prefetch the  ephemeris file to the main memory
  @param eph (in) ephemeris descriptor (as integer for F77 compatibility)
*/
/********************************************************/
int FC_FUNC_(f90calceph_prefetch, F90CALCEPH_PREFETCH) (long long *eph)
{
    return calceph_prefetch(f90calceph_getaddresseph(eph));
}

/********************************************************/
/*! Return a non-zero value if the  ephemeris file could be accessed by multiple threads at the same time
  @param eph (in) ephemeris descriptor (as integer for F77 compatibility)
*/
/********************************************************/
int FC_FUNC_(f90calceph_isthreadsafe, F90CALCEPH_ISTHREADSAFE) (long long *eph)
{
    return calceph_isthreadsafe(f90calceph_getaddresseph(eph));
}

/********************************************************/
/*! Return the position and the velocity of the object
  "target" in the vector PV at the specified time
  (time elapsed from JD0).
  The center is specified in center.
  The output is expressed in AU, AU/day, RAD/day and for TT-TDB in seconds.

  return 0 on error.
  return 1 on success.

   @param eph (in) ephemeris descriptor (as integer for F77 compatibility)
   @param JD0 (in) reference time (could be 0)
   @param time (in) time elapsed from JD0
   @param target (in) "target" object
   @param center (in) "center" object
   @param PV (out) position and the velocity of the "target" object
*/
/********************************************************/
int FC_FUNC_(f90calceph_compute, F90CALCEPH_COMPUTE) (long long *eph, double *JD0, double *time,
                                                      int *target, int *center, double PV[6])
{
    return calceph_compute(f90calceph_getaddresseph(eph), *JD0, *time, *target, *center, PV);
}

/********************************************************/
/*! Return the position and the velocity of the object
 "target" in the vector PV at the specified time
 (time elapsed from JD0).
 The center is specified in center.
 The output is expressed according to unit.

 return 0 on error.
 return 1 on success.

 @param eph (in) ephemeris descriptor (as integer for F77 compatibility)
 @param JD0 (in) reference time (could be 0)
 @param time (in) time elapsed from JD0
 @param target (in) "target" object
 @param center (in) "center" object
 @param unit (in) units of PV ( combination of CALCEPH_UNIT_??? )  and/or
 CALCEPH_USE_NAIFID
 @param order (in) order of the computation
 =0 : Positions are  computed
 =1 : Position+Velocity  are computed
 =2 : Position+Velocity+Acceleration  are computed
 =3 : Position+Velocity+Acceleration+Jerk  are computed
 @param PVAJ (out) orientation (euler angles and their derivatives) of the
 "target" object This array must be able to contain at least 3*(order+1)
 floating-point numbers. On exit, PVAJ[0:3*(order+1)-1] contain valid
 floating-point numbers.
 */
/********************************************************/
int FC_FUNC_(f90calceph_compute_order, F90CALCEPH_COMPUTE_ORDER) (long long *eph, double *JD0,
                                                                  double *time, int *target, int *center,
                                                                  int *unit, int *order, double *PVAJ)
{
    return calceph_compute_order(f90calceph_getaddresseph(eph), *JD0, *time, *target, *center, *unit,
                                 *order, PVAJ);
}

/********************************************************/
/*! Return the orientation of the object
 "target" in the vector PV at the specified time
 (time elapsed from JD0).
 The output is expressed according to unit.

 return 0 on error.
 return 1 on success.

 @param eph (in) ephemeris descriptor (as integer for F77 compatibility)
 @param JD0 (in) reference time (could be 0)
 @param time (in) time elapsed from JD0
 @param target (in) "target" object
 @param unit (in) units of PV ( combination of CALCEPH_UNIT_??? ) and/or
 CALCEPH_USE_NAIFID
 @param order (in) order of the computation
 =0 : Positions are  computed
 =1 : Position+Velocity  are computed
 =2 : Position+Velocity+Acceleration  are computed
 =3 : Position+Velocity+Acceleration+Jerk  are computed
 @param PVAJ (out) orientation (euler angles and their derivatives) of the
 "target" object This array must be able to contain at least 3*(order+1)
 floating-point numbers. On exit, PVAJ[0:3*(order+1)-1] contain valid
 floating-point numbers.
 */
/********************************************************/
int FC_FUNC_(f90calceph_orient_order, F90CALCEPH_ORIENT_ORDER) (long long *eph, double *JD0, double *time,
                                                                int *target, int *unit, int *order, double *PVAJ)
{
    return calceph_orient_order(f90calceph_getaddresseph(eph), *JD0, *time, *target, *unit, *order, PVAJ);
}

/*--------------------------------------------------------------------------*/
/*! Return the rotational angular momentum (G/mR^2) of the object
 "target" in the vector PV at the specified time
 (time elapsed from JD0).
 The output is expressed according to unit.

 return 0 on error.
 return 1 on success.

 @param eph (in) ephemeris descriptor
 @param JD0 (in) reference time (could be 0)
 @param time (in) time elapsed from JD0
 @param target (in) "target" object
 @param unit (in) units of PV ( combination of CALCEPH_UNIT_??? ) and/or
 CALCEPH_USE_NAIFID
 @param order (in) order of the computation
 =0 : (G/mR^2) are computed
 =1 : (G/mR^2) and their first derivatives are computed
 =2 : (G/mR^2) and their first,second derivatives  are computed
 =3 : (G/mR^2) and their first, second, third derivatives  are computed
 @param PVAJ (out) rotational angular momentum (G/mR^2) and their derivatives of
 the "target" object This array must be able to contain at least 3*(order+1)
 floating-point numbers. On exit, PVAJ[0:3*(order+1)-1] contain valid
 floating-point numbers.
 */
/*--------------------------------------------------------------------------*/
int FC_FUNC_(f90calceph_rotangmom_order, F90CALCEPH_ROTANGMOM_ORDER) (long long *eph, double *JD0,
                                                                      double *time, int *target,
                                                                      int *unit, int *order, double *PVAJ)
{
    return calceph_rotangmom_order(f90calceph_getaddresseph(eph), *JD0, *time, *target, *unit, *order,
                                   PVAJ);
}

/********************************************************/
/*! Return the position and the velocity of the object
  "target" in the vector PV at the specified time
  (time elapsed from JD0).
  The center is specified in center.
  The output is expressed according to unit.

  return 0 on error.
  return 1 on success.

   @param eph (in) ephemeris descriptor (as integer for F77 compatibility)
   @param JD0 (in) reference time (could be 0)
   @param time (in) time elapsed from JD0
   @param target (in) "target" object
   @param center (in) "center" object
   @param unit (in) units of PV ( combination of CALCEPH_UNIT_??? )  and/or
  CALCEPH_USE_NAIFID
   @param PV (out) position and the velocity of the "target" object
*/
/********************************************************/
int FC_FUNC_(f90calceph_compute_unit, F90CALCEPH_COMPUTE_UNIT) (long long *eph, double *JD0, double *time,
                                                                int *target, int *center, int *unit, double PV[6])
{
    return calceph_compute_unit(f90calceph_getaddresseph(eph), *JD0, *time, *target, *center, *unit, PV);
}

/********************************************************/
/*! Return the orientation of the object
  "target" in the vector PV at the specified time
  (time elapsed from JD0).
  The output is expressed according to unit.

  return 0 on error.
  return 1 on success.

   @param eph (in) ephemeris descriptor (as integer for F77 compatibility)
   @param JD0 (in) reference time (could be 0)
   @param time (in) time elapsed from JD0
   @param target (in) "target" object
   @param unit (in) units of PV (combination of CALCEPH_UNIT_??? ) and/or
  CALCEPH_USE_NAIFID
   @param PV (out) orientation (euler angles and their derivatives) of the
  "target" object
*/
/********************************************************/
int FC_FUNC_(f90calceph_orient_unit, F90CALCEPH_ORIENT_UNIT) (long long *eph, double *JD0, double *time,
                                                              int *target, int *unit, double PV[6])
{
    return calceph_orient_unit(f90calceph_getaddresseph(eph), *JD0, *time, *target, *unit, PV);
}

/*--------------------------------------------------------------------------*/
/*! Return the rotational angular momentum (G/mR^2) of the object
  "target" in the vector PV at the specified time
  (time elapsed from JD0).
  The output is expressed according to unit.

  return 0 on error.
  return 1 on success.

   @param eph (in) ephemeris descriptor
   @param JD0 (in) reference time (could be 0)
   @param time (in) time elapsed from JD0
   @param target (in) "target" object
   @param unit (in) units of PV ( combination of CALCEPH_UNIT_??? ) and/or
  CALCEPH_USE_NAIFID
   @param PV (out) rotational angular momentum (G/mR^2) and their derivatives of
  the "target" object
*/
/*--------------------------------------------------------------------------*/
int FC_FUNC_(f90calceph_rotangmom_unit, F90CALCEPH_ROTANGMOM_UNIT) (long long *eph, double *JD0,
                                                                    double *time, int *target, int *unit, double PV[6])
{
    return calceph_rotangmom_unit(f90calceph_getaddresseph(eph), *JD0, *time, *target, *unit, PV);
}

/********************************************************/
/*! return the number of constants available in the ephemeris file

  @param eph (inout) ephemeris descriptor
*/
/********************************************************/
int FC_FUNC_(f90calceph_getconstantcount, F90CALCEPH_GETCONSTANTCOUNT) (long long *eph)
{
    return calceph_getconstantcount(f90calceph_getaddresseph(eph));
}

/********************************************************/
/*! return the name and the associated value of the constant available
    at some index from the ephemeris file
   return 1 if index is valid
   return 0 if index isn't valid

  @param eph (inout) ephemeris descriptor
  @param index (in) index of the constant (must be between 1 and
  calceph_getconstantcount() )
  @param name (out) name of the constant
  @param value (out) value of the constant
*/
/********************************************************/
int FC_FUNC_(f90calceph_getconstantindex,
             F90CALCEPH_GETCONSTANTINDEX) (long long *eph, int *index,
                                           char name[CALCEPH_MAX_CONSTANTNAME], double *value)
{
    int res = calceph_getconstantindex(f90calceph_getaddresseph(eph), *index, name, value);

    if (res == 1)
    {
        calceph_fortranfillspace(name, CALCEPH_MAX_CONSTANTNAME);
    }
    return res;
}

/********************************************************/
/*! return the name and the associated value of the constant available
    at some index from the ephemeris file
   return 1 if index is valid
   return 0 if index isn't valid

  @param eph (inout) ephemeris descriptor
  @param index (in) index of the constant (must be between 1 and
  calceph_getconstantcount() )
  @param name (out) name of the constant
  @param value (out) value of the constant
*/
/********************************************************/
int f2003calceph_getconstantindex(t_calcephbin * eph, int index, char name[CALCEPH_MAX_CONSTANTNAME], double *value)
{
    int k;

    int res = calceph_getconstantindex(eph, index, name, value);

    if (res == 1)
    {
        for (k = 0; k < CALCEPH_MAX_CONSTANTNAME - 1; k++)
        {
            if (name[k] == '\0')
                name[k] = ' ';
        }
    }
    return res;
}

/********************************************************/
/*! return the value of the specified constant frothe ephemeris file
  return 0 if constant is not found
  return 1 if constant is found and set value.

  @param name (inout) constant name
  @param value (out) value of the constant
*/
/********************************************************/
int FC_FUNC_(f90calceph_getconstant, F90CALCEPH_GETCONSTANT) (long long *eph, char *name, double *value, long int len)
{
    int res = 0;

    char *newname;

    newname = (char *) malloc(sizeof(char) * (len + 1));
    if (newname != NULL)
    {
        memcpy(newname, name, len * sizeof(char));
        newname[len] = '\0';
        res = calceph_getconstant(f90calceph_getaddresseph(eph), newname, value);
        free(newname);
    }
    /* GCOVR_EXCL_START */
    else
    {
        buffer_error_t buffer_error;

        fatalerror("Can't allocate memory for f90calceph_getconstant\nSystem error "
                   ": '%s'\n", calceph_strerror_errno(buffer_error));
    }
    /* GCOVR_EXCL_STOP */
    return res;
}

/********************************************************/
/*! return the value of the specified constant from the ephemeris file
  return 0 if constant is not found
  return 1 if constant is found and set value.

  @param name (inout) constant name
  @param value (out) value of the constant
  @param len (in) length of name
*/
/********************************************************/
int FC_FUNC_(f90calceph_getconstantsd, F90CALCEPH_GETCONSTANTSD) (long long *eph, char *name,
                                                                  double *value, long int len)
{
    int res = 0;

    char *newname;

    newname = (char *) malloc(sizeof(char) * (len + 1));
    if (newname != NULL)
    {
        memcpy(newname, name, len * sizeof(char));
        newname[len] = '\0';
        res = calceph_getconstantsd(f90calceph_getaddresseph(eph), newname, value);
        free(newname);
    }
    /* GCOVR_EXCL_START */
    else
    {
        buffer_error_t buffer_error;

        fatalerror("Can't allocate memory for f90calceph_getconstantsd\nSystem "
                   "error : '%s'\n", calceph_strerror_errno(buffer_error));
    }
    /* GCOVR_EXCL_STOP */
    return res;
}

/********************************************************/
/*! fill the array arrayvalue with the nvalue first values of the specified
  constant from the text PCK kernel return 0 if constant is not found return the
  number of values associated to the constant (this value may be larger than
  nvalue), if constant is  found
  @param eph (in) ephemeris file
  @param name (in) constant name
  @param name (in) constant name
  @param arrayvalue (out) array of values of the constant.
    On input, the size of the array is nvalue elements.
    On exit, the min(returned value, nvalue) elements are filled.
  @param nvalue (in) number of elements in the array arrayvalue.
  @param len (in) length of name
*/
/********************************************************/
int FC_FUNC_(f90calceph_getconstantvd, F90CALCEPH_GETCONSTANTVD) (long long *eph, char *name,
                                                                  double *arrayvalue, const int *nvalue, long int len)
{
    int res = 0;

    char *newname;

    newname = (char *) malloc(sizeof(char) * (len + 1));
    if (newname != NULL)
    {
        memcpy(newname, name, len * sizeof(char));
        newname[len] = '\0';
        res = calceph_getconstantvd(f90calceph_getaddresseph(eph), newname, arrayvalue, *nvalue);
        free(newname);
    }
    /* GCOVR_EXCL_START */
    else
    {
        buffer_error_t buffer_error;

        fatalerror("Can't allocate memory for f90calceph_getconstantvd\nSystem "
                   "error : '%s'\n", calceph_strerror_errno(buffer_error));
    }
    /* GCOVR_EXCL_STOP */
    return res;
}

/********************************************************/
/*! return the value of the specified constant from the ephemeris file
  return 0 if constant is not found
  return 1 if constant is found and set value.

  @param name (inout) constant name
  @param value (out) value of the constant
  @param len (in) length of name
*/
/********************************************************/
int FC_FUNC_(f90calceph_getconstantss, F90CALCEPH_GETCONSTANTSS) (long long *eph, char *name,
                                                                  t_calcephcharvalue value, long int len)
{
    int res = 0;

    char *newname;

    newname = (char *) malloc(sizeof(char) * (len + 1));
    if (newname != NULL)
    {
        memcpy(newname, name, len * sizeof(char));
        newname[len] = '\0';
        res = calceph_getconstantss(f90calceph_getaddresseph(eph), newname, value);
        if (res >= 1)
        {
            calceph_fortranfillspace(value, CALCEPH_MAX_CONSTANTVALUE);
        }
        free(newname);
    }
    /* GCOVR_EXCL_START */
    else
    {
        buffer_error_t buffer_error;

        fatalerror("Can't allocate memory for f90calceph_getconstantss\nSystem "
                   "error : '%s'\n", calceph_strerror_errno(buffer_error));
    }
    /* GCOVR_EXCL_STOP */
    return res;
}

/********************************************************/
/*! fill the array arrayvalue with the nvalue first values of the specified
  constant from the text PCK kernel return 0 if constant is not found return the
  number of values associated to the constant (this value may be larger than
  nvalue), if constant is  found
  @param eph (in) ephemeris file
  @param name (in) constant name
  @param name (in) constant name
  @param arrayvalue (out) array of values of the constant.
    On input, the size of the array is nvalue elements.
    On exit, the min(returned value, nvalue) elements are filled.
  @param nvalue (in) number of elements in the array arrayvalue.
  @param len (in) length of name
*/
/********************************************************/
int FC_FUNC_(f90calceph_getconstantvs, F90CALCEPH_GETCONSTANTVS) (long long *eph, char *name,
                                                                  t_calcephcharvalue * arrayvalue,
                                                                  const int *nvalue, long int len)
{
    int res = 0, k;

    char *newname;

    newname = (char *) malloc(sizeof(char) * (len + 1));
    if (newname != NULL)
    {
        memcpy(newname, name, len * sizeof(char));
        newname[len] = '\0';
        res = calceph_getconstantvs(f90calceph_getaddresseph(eph), newname, arrayvalue, *nvalue);
        for (k = 0; k < res; k++)
        {
            calceph_fortranfillspace(arrayvalue[k], CALCEPH_MAX_CONSTANTVALUE);
        }
        free(newname);
    }
    /* GCOVR_EXCL_START */
    else
    {
        buffer_error_t buffer_error;

        fatalerror("Can't allocate memory for f90calceph_getconstantvs\nSystem "
                   "error : '%s'\n", calceph_strerror_errno(buffer_error));
    }
    /* GCOVR_EXCL_STOP */
    return res;
}

/********************************************************/
/*! return the time scale used in the ephemeris file

  @param eph (inout) ephemeris descriptor
*/
/********************************************************/
int FC_FUNC_(f90calceph_gettimescale, F90CALCEPH_GETTIMESCALE) (long long *eph)
{
    return calceph_gettimescale(f90calceph_getaddresseph(eph));
}

/********************************************************/
/*! This function returns the first and last time available in the ephemeris
file associated to eph. The Julian date for the first and last time are
expressed in the time scale returned by calceph_gettimescale().

  @param eph (inout) ephemeris descriptor
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
/********************************************************/
int FC_FUNC_(f90calceph_gettimespan, F90CALCEPH_GETTIMESPAN) (long long *eph, double *firsttime,
                                                              double *lasttime, int *continuous)
{
    return calceph_gettimespan(f90calceph_getaddresseph(eph), firsttime, lasttime, continuous);
}

/********************************************************/
/*! store, in id, the identification number of the body, given its name.
   return 0 if the name was not found.
   return 1 on sucess.

  @param eph (inout) ephemeris descriptor
  @param name (in) null-terminated string of the name
  @param unit (in) 0 or CALCEPH_USE_NAIFID
  @param id (out) identification number.
     The value depends on the numbering system given by value.
*/
/********************************************************/
int FC_FUNC_(f90calceph_getidbyname, F90CALCEPH_GETIDBYNAME) (long long *eph, char *name, int *unit,
                                                              int *id, long int len)
{
    int res = 0;

    char *newname;

    newname = (char *) malloc(sizeof(char) * (len + 1));
    if (newname != NULL)
    {
        memcpy(newname, name, len * sizeof(char));
        newname[len] = '\0';
        res = calceph_getidbyname(f90calceph_getaddresseph(eph), newname, *unit, id);
        free(newname);
    }
    /* GCOVR_EXCL_START */
    else
    {
        buffer_error_t buffer_error;

        fatalerror("Can't allocate memory for f90calceph_getidbyname\nSystem error : '%s'\n",
                   calceph_strerror_errno(buffer_error));
    }
    /* GCOVR_EXCL_STOP */

    return res;
}

/********************************************************/
/*! return the value of the specified constant from the ephemeris file
  return 0 if constant is not found
  return 1 if constant is found and set value.

  @param name (inout) constant name
  @param value (out) value of the constant
*/
/********************************************************/
int FC_FUNC_(f90calceph_getnamebyidss, F90CALCEPH_GETNAMEBYIDSS) (long long *eph, int *id, int *unit,
                                                                  t_calcephcharvalue value)
{
    int res = 0;

    value[0] = '\0';
    res = calceph_getnamebyidss(f90calceph_getaddresseph(eph), *id, *unit, value);
    if (res >= 1)
    {
        calceph_fortranfillspace(value, CALCEPH_MAX_CONSTANTVALUE);
    }

    return res;
}

/********************************************************/
/*! return the number of position’s records available in the ephemeris file

  @param eph (inout) ephemeris descriptor
*/
/********************************************************/
int FC_FUNC_(f90calceph_getpositionrecordcount, F90CALCEPH_GETPOSITIONRECORDCOUNT) (long long *eph)
{
    return calceph_getpositionrecordcount(f90calceph_getaddresseph(eph));
}

/********************************************************/
/*! This function returns the target and origin bodies, the first and last time,
 and the reference frame available at the specified index
 for the position's records of the ephemeris file  associated to eph.

   return 0 on error, otherwise non-zero value.

  @param eph (inout) ephemeris descriptor
  @param index (in) index of the position’s record, between 1 and
 calceph_getpositionrecordcount()
  @param target – The target body
  @param center – The origin body
  @param firsttime (out) the Julian date of the first time available in this
 record, expressed in the same time scale as calceph_gettimescale.
  @param lasttime (out) the Julian date of the first time available in this
 record, expressed in the same time scale as calceph_gettimescale.
  @param frame (out) reference frame
  1  = ICRF
*/
/********************************************************/
int FC_FUNC_(f90calceph_getpositionrecordindex,
             F90CALCEPH_GETPOSITIONRECORDINDEX) (long long *eph, int *index, int *target, int *center,
                                                 double *firsttime, double *lasttime, int *frame)
{
    return calceph_getpositionrecordindex(f90calceph_getaddresseph(eph), *index, target, center, firsttime,
                                          lasttime, frame);
}

/********************************************************/
/*! This function returns the target and origin bodies, the first and last time,
 , the reference frame and the segment type available at the specified index
 for the position's records of the ephemeris file  associated to eph.

   return 0 on error, otherwise non-zero value.

  @param eph (inout) ephemeris descriptor
  @param index (in) index of the position’s record, between 1 and
 calceph_getpositionrecordcount()
  @param target – The target body
  @param center – The origin body
  @param firsttime (out) the Julian date of the first time available in this
 record, expressed in the same time scale as calceph_gettimescale.
  @param lasttime (out) the Julian date of the first time available in this
 record, expressed in the same time scale as calceph_gettimescale.
  @param frame (out) reference frame
  1  = ICRF
  @param segtype (out) type of segment
*/
/********************************************************/
int FC_FUNC_(f90calceph_getpositionrecordindex2,
             F90CALCEPH_GETPOSITIONRECORDINDEX2) (long long *eph, int *index, int *target, int *center,
                                                  double *firsttime, double *lasttime, int *frame, int *segtype)
{
    return calceph_getpositionrecordindex2(f90calceph_getaddresseph(eph), *index, target, center,
                                           firsttime, lasttime, frame, segtype);
}

/********************************************************/
/*! return the number of position’s records available in the ephemeris file

  @param eph (inout) ephemeris descriptor
*/
/********************************************************/
int FC_FUNC_(f90calceph_getorientrecordcount, F90CALCEPH_GETORIENTRECORDCOUNT) (long long *eph)
{
    return calceph_getorientrecordcount(f90calceph_getaddresseph(eph));
}

/********************************************************/
/*! This function returns the target and origin bodies, the first and last time,
 and the reference frame available at the specified index
 for the orientation's records of the ephemeris file  associated to eph.

   return 0 on error, otherwise non-zero value.

  @param eph (inout) ephemeris descriptor
  @param index (in) index of the position’s record, between 1 and
 calceph_getporientrecordcount()
  @param target – The target body
  @param firsttime (out) the Julian date of the first time available in this
 record, expressed in the same time scale as calceph_gettimescale.
  @param lasttime (out) the Julian date of the first time available in this
 record, expressed in the same time scale as calceph_gettimescale.
  @param frame (out) reference frame
  1  = ICRF
*/
/********************************************************/
int FC_FUNC_(f90calceph_getorientrecordindex,
             F90CALCEPH_GETPOSITIONRECORDINDEX) (long long *eph, int *index, int *target,
                                                 double *firsttime, double *lasttime, int *frame)
{
    return calceph_getorientrecordindex(f90calceph_getaddresseph(eph), *index, target, firsttime, lasttime,
                                        frame);
}

/********************************************************/
/*! This function returns the target and origin bodies, the first and last time,
 the reference frame, and the segment type available at the specified index
 for the orientation's records of the ephemeris file  associated to eph.

   return 0 on error, otherwise non-zero value.

  @param eph (inout) ephemeris descriptor
  @param index (in) index of the position’s record, between 1 and
 calceph_getporientrecordcount()
  @param target – The target body
  @param firsttime (out) the Julian date of the first time available in this
 record, expressed in the same time scale as calceph_gettimescale.
  @param lasttime (out) the Julian date of the first time available in this
 record, expressed in the same time scale as calceph_gettimescale.
  @param frame (out) reference frame
  1  = ICRF
  @param segid (out) type of segment
*/
/********************************************************/
int FC_FUNC_(f90calceph_getorientrecordindex2,
             F90CALCEPH_GETPOSITIONRECORDINDEX2) (long long *eph, int *index, int *target,
                                                  double *firsttime, double *lasttime, int *frame, int *segid)
{
    return calceph_getorientrecordindex2(f90calceph_getaddresseph(eph), *index, target, firsttime,
                                         lasttime, frame, segid);
}

/********************************************************/
/*! store, in version, the file version of the ephemeris file.
   return 0 if the file version was not found.
   return 1 on sucess.

  @param eph (inout) ephemeris descriptor
  @param version (out) fortran string of the version of the ephemeris
  file

*/
/********************************************************/
int FC_FUNC_(f90calceph_getfileversion,
             F90CALCEPH_GETFILEVERSION) (long long *eph, char version[CALCEPH_MAX_CONSTANTVALUE])
{
    return f2003calceph_getfileversion(f90calceph_getaddresseph(eph), version);
}

/********************************************************/
/*! store, in version, the file version of the ephemeris file.
   return 0 if the file version was not found.
   return 1 on sucess.

  @param eph (inout) ephemeris descriptor
  @param szversion (out) fortran string of the version of the ephemeris
  file

*/
/********************************************************/
int f2003calceph_getfileversion(t_calcephbin * eph, char version[CALCEPH_MAX_CONSTANTVALUE])
{
    int res = calceph_getfileversion(eph, version);

    calceph_fortranfillspace(version, CALCEPH_MAX_CONSTANTVALUE);
    return res;
}

/********************************************************/
/*!  set the error handler for the calceph library

  @param type (in) possible value :
             1 : prints the message and the program continues (default)
             2 : prints the message and the program exits (call exit(1))
             3 : call the function userfunc and the programm continues
  @param userfunc (in) pointer to a function
             useful only if type = 3
             This function is called on error with a message as the first
  argument
*/
/********************************************************/
void FC_FUNC_(f90calceph_seterrorhandler, F90CALCEPH_SETERRORHANDLER) (int *type, void (*userfunc) (const char *))
{
    if (*type == 1 || *type == 2)
        userfunc = NULL;
    calceph_seterrorhandler(*type, userfunc);
}

/********************************************************/
/* replace the character \0 and after it by spaces

 @param name (inout) string to be processed
 On input, the string is a null-terminated string (C string)
 On exit, the string is compliant with fortran string.
 On exit, the string is no longer a null-terminated string.
 @param len (in) size of the buffer for name

*/
/********************************************************/
static void calceph_fortranfillspace(char *name, size_t len)
{
    size_t j;

    for (j = strlen(name); j < len; j++)
        name[j] = ' ';
}

/********************************************************/
/*! return the version as a string.
  The trailing character are filled with a space character.

  @param version (out) name of the version
*/
/********************************************************/
void f2003calceph_getversion_str(char version[CALCEPH_MAX_CONSTANTNAME])
{
    calceph_getversion_str(version);
    calceph_fortranfillspace(version, CALCEPH_MAX_CONSTANTNAME);
}

/********************************************************/
/*! return the version as a string.
  The trailing character are filled with a space character.

  @param version (out) name of the version
*/
/********************************************************/
void FC_FUNC_(f90calceph_getversion_str, F90CALCEPH_GETVERSION_STR) (char version[CALCEPH_MAX_CONSTANTNAME])
{
    f2003calceph_getversion_str(version);
}

/********************************************************/
/*! return, for a given type of segment, the maximal order of the derivatives
  It returns -1 if the segment type is unknown or not supported by the library

  @param idseg (in) number of the segment
*/
/********************************************************/
int FC_FUNC_(f90calceph_getmaxsupportedorder, F90CALCEPH_GETMAXSUPPORTEDORDER) (int *idseg)
{
    return calceph_getmaxsupportedorder(*idseg);
}

/********************************************************/
/*! Open the specified ephemeris file
  return 0 on error.
  return 1 on success.

 @param filename (in) nom du fichier
*/
/********************************************************/
int calcephinit_(char *filename, long int len)
{
    return FC_FUNC_(f90calceph_sopen, F90CALCEPH_SOPEN) (filename, len);
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
int calceph_(double *JD0, double *time, int *target, int *center, double PV[6])
{
    return calceph_scompute(*JD0, *time, *target, *center, PV);
}
