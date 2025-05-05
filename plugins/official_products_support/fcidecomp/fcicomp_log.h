// =============================================================
//
// Copyright 2015-2023, European Organisation for the Exploitation of Meteorological Satellites (EUMETSAT)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// =============================================================

// AUTHORS:
// - THALES Services

/*! \file

 This file define the logging function of fcicomp.

 */

#ifndef FCICOMP_LOG_H_
#define FCICOMP_LOG_H_

/**@{*/
/** Define logging message severity by decreasing order of severity.*/
typedef enum {
	ERROR_SEVERITY = 0,
	WARNING_SEVERITY,
	NORMAL_SEVERITY,
	DEBUG_SEVERITY
} msg_severity_t;
/**@}*/

#define LOGGING
#ifdef LOGGING

/** Define the default logging level */
#define DEFAULT_LOGGING_LEVEL	ERROR_SEVERITY

/* Set the logging level to the default value if it has not been defined at
 * building time */
#ifndef LOGGING_LEVEL
/** Define the logging level */
#define LOGGING_LEVEL			DEFAULT_LOGGING_LEVEL
#endif /* LOGGING_LEVEL */

///**
// * @brief Print a logging message.
// *
// * Normal message are printed on the stdout. Warning and error message are
// * printed on the stderr.
// *
// * This function is heavily based on the function nc_log in netCDF-4 file error4.c.
// */
void fcicomp_log(msg_severity_t severity, const char *fmt, ...);

#define LOG(s, ...) 			(fcicomp_log ((s), __VA_ARGS__))

#else /* LOGGING */

/* This definition will be used unless LOGGING is defined. */
// #define LOG(s, ...)
static void LOG (msg_severity_t s, ...)

#endif /* LOGGING */

/* Handle errors by printing an error message and exiting with a non-zero status. */
// #define ERR(e, msg) {LOG(ERROR_SEVERITY, (msg)); return (e);}
static int ERR(int e, char * msg) {LOG(ERROR_SEVERITY, (msg)); return (e);}

#endif /* FCICOMP_LOG_H_ */
