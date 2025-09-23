/*-----------------------------------------------------------------*/
/*! 
  \file util.h 
  \brief supplementary tools.

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

#ifndef __UTIL_H__
#define __UTIL_H__

/*enable unused pragma for : clang, intel   */
#if defined(__clang__) || defined(__INTEL_COMPILER)
#ifndef HAVE_PRAGMA_UNUSED
#define HAVE_PRAGMA_UNUSED 1
#endif
#else
/*disable unknown pragma for pragma unused if not available  */
#ifndef HAVE_PRAGMA_UNUSED
#define HAVE_PRAGMA_UNUSED 0
#endif
#endif

/*disable warning for intel compilers : warning #161: unrecognized #pragma  */
#ifdef __INTEL_COMPILER
#pragma warning(disable:161)
#endif

#if HAVE_PRAGMA_UNUSED

/*disable warning for gcc compilers : unrecognized #pragma  */
#pragma GCC diagnostic ignored "-Wunknown-pragmas"

/* disable warning : warning "operands are evaluated in unspecified order" */
#pragma warning(disable:981)

#endif

/* set the unused attribute for the parameters */
#ifdef HAVE_VAR_ATTRIBUTE_UNUSED
#define PARAMETER_UNUSED(x)  x __attribute__((__unused__))
#else
#define PARAMETER_UNUSED(x)  x
#endif

void calceph_fatalerror(const char *format, ...);

#define fatalerror calceph_fatalerror

int swapint(int x);
double swapdbl(double x);
void swapintarray(int *x, int count);
void swapdblarray(double *x, size_t count);

#if !defined(HAVE_RINT)
double rint(double x);
#endif
#if !defined(HAVE_VASPRINTF)
#ifdef STDC_HEADERS
#include <stdarg.h>
#endif
int vasprintf(char **ret, const char *format, va_list args);
#endif

#if defined(HAVE_SNPRINTF)
#define calceph_snprintf snprintf
#else
int calceph_snprintf(char * str, size_t size, const char * format, ...);
#endif

/* platform without fseeko or ftello functions : android api <=24 */
#ifndef HAVE_FSEEKO
#define fseeko fseek
#define ftello ftell
#endif

/*! definitions for thread-safe massages with  calceph_strerror_errno(buffer_error) */
#define BUFFERSIZE_STRERROR 512
typedef char buffer_error_t[BUFFERSIZE_STRERROR];
const char *calceph_strerror_errno(buffer_error_t buffer);

/*! ensure to call isspace with unsigned char encoding */
#define isspace_char(x) isspace((unsigned char)(x))
/*! ensure to call isdigit with unsigned char encoding */
#define isdigit_char(x) isdigit((unsigned char)(x))

#endif /*__UTIL_H__*/
