
/* Define to a macro mangling the given C identifier (in lower and upper
   case), which must not contain underscores, for linking with Fortran. */
#define FC_FUNC(name,NAME) name ## _

/* As FC_FUNC, but for C identifiers containing underscores. */
#define FC_FUNC_(name,NAME) name ## _

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1 

/* Define to 1 if you have the `freelocale' function. */
#define HAVE_FREELOCALE 1 

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
#define HAVE_FSEEKO 1 

/* Define to 1 if _fseeki64 (and presumably ftello) exists and is declared. */
/* #undef HAVE__FSEEKI64 */

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1 

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1 

/* Define to 1 if you have the <math.h> header file. */
#define HAVE_MATH_H 1 

/* Define to 1 if you have a working `mmap' system call. */
#define HAVE_MMAP 1 
#if defined(_WIN32)
#include "libs/mmap_windows.h"
#else
#include <sys/mman.h>
#endif

/* Define to 1 if you have the `newlocale' function. */
#define HAVE_NEWLOCALE 1 

/* Define if you have POSIX threads libraries and header files. */
#define HAVE_PTHREAD 1 

/* define it to 1 if rint is available */
#define HAVE_RINT 1 

/* Define to 1 if you have the `setlocale' function. */
#define HAVE_SETLOCALE 1 

/* Define to 1 if you have the <stdbool.h> header file. */
#define HAVE_STDBOOL_H 1 

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1 

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1 

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1 

/* Define to 1 if you have the `strncmp' function. */
#define HAVE_STRNCMP 1 

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1 
#ifdef _WIN32
#include "libs/strings_h_win.h"
#endif

/* Define to 1 if you have the `_stricmp' function. */
/* #undef HAVE__STRICMP */

/* Define to 1 if you have the `_strtod_l' function. */
/* #undef HAVE__STRTOD_L */

/* Define to 1 if you have the `strtod_l' function. */
#define HAVE_STRTOD_L 0 

/* Define to 1 if you have the <sys/mman.h> header file. */
// #define HAVE_SYS_MMAN_H 1 

/* Define to 1 if you have the <sys/resource.h> header file. */
#define HAVE_SYS_RESOURCE_H 1 

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1 

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1 

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1 

/* Define to 1 if you have the <unistd.h> header file. */
// #define HAVE_UNISTD_H 1 

/* Define to 1 if the system has the `unused' variable attribute */
// #define HAVE_VAR_ATTRIBUTE_UNUSED 1 

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1 

/* Define to 1 if you have the `vasprintf' function. */
// #define HAVE_VASPRINTF 1

/* Define to 1 if you have the `_vscprintf' function. */
/* #undef HAVE__VSCPRINTF */

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1 

/* Define to 1 if you have the <xlocale.h> header file. */
/* #undef HAVE_XLOCALE_H */

/* Define to 1 if you have the `strerror_r' function. */
// #define HAVE_STRERROR_R 1

/* Define to 1 if you have the `strerror_s' function. */
/* #undef HAVE_STRERROR_S */

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1 

/* Define to 1 if bool is available. */
#define HAVE_BOOL 1

/* Define to 1 if you have the <windows.h> header file */
/* #undef HAVE_WINDOWS_H */

#if HAVE_WINDOWS_H
#include <windows.h>
#define HAVE_WIN32API 1
#endif

#ifndef HAVE_BOOL
#define bool BOOL
#define false FALSE
#define true TRUE
#endif

#if !defined(HAVE_FSEEKO) && defined(HAVE__FSEEKI64)
#define fseeko _fseeki64
#define ftello _ftelli64
#undef HAVE_FSEEKO
#define HAVE_FSEEKO 1
#endif

#if !defined(HAVE_STRCASECMP) && defined(HAVE__STRICMP)
#define strcasecmp _stricmp
#undef HAVE_STRCASECMP
#define HAVE_STRCASECMP 1
#endif

#if !defined(HAVE_STRTOD_L) && defined(HAVE__STRTOD_L)
#define strtod_l _strtod_l
#undef HAVE_STRTOD_L
#define HAVE_STRTOD_L 1
#endif


#if defined(__MINGW32__) || defined (__MINGW64__) || defined(_MSC_VER)
#if (__MSVCRT_VERSION__<0x0800 &&  (defined(__MINGW32__) || defined (__MINGW64__))) || defined(_WIN32)
/* mingw defined _fseeki64 only for __MSVCRT_VERSION__>=0x0800 */
#define fseeko fseek
#define ftello ftell
#undef HAVE_FSEEKO
#define HAVE_FSEEKO 1
#endif


#if defined(__GNUC__) &&  !(defined(__MINGW32__) || defined (__MINGW64__))
#undef fseeko 
#undef ftello
#undef HAVE_FSEEKO
#endif

#ifdef _MSC_VER
#define locale_t _locale_t
#define HAVE__CREATE_LOCALE 1
#define freelocale _free_locale
#undef HAVE__VSCPRINTF
#define HAVE__VSCPRINTF 1
#endif

#if !defined(__cplusplus) && !defined(__GNUC__)
#define inline _inline
#endif /*__cplusplus*/


#endif /*defined(__MINGW32__) || defined (__MINGW64__) || defined(_MSC_VER)*/
