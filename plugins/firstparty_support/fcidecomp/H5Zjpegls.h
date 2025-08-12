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

#ifndef H5ZJPEGLS_H_
#define H5ZJPEGLS_H_

#include "fcicomp_jpegls.h"

/**
 * Define the filter ID.
 *
 * The filter identifier is designed to be a unique identifier for the filter.
 * Values from zero through 32,767 are reserved for filters supported by The
 * HDF Group in the HDF5 library and for filters requested and supported by
 * the 3rd party.
 *
 * Values from 32768 to 65535 are reserved for non-distributed uses (e.g.,
 * internal company usage) or for application usage when testing a feature.
 * The HDF Group does not track or document the usage of filters with
 * identifiers from this range.
 *
 * The filter ID has been provided by the HDF Group.
 */
#define H5Z_FILTER_JPEGLS			32018

/** Define the filter name */
#define H5Z_FILTER_JPEGLS_NAME		"JPEG-LS"

/** Maximum number of dimensions of the datasets compressed with the JPEG-LS filter.
 *
 * Allow color images.
 */
#define H5Z_FILTER_JPEGLS_MAX_NDIMS			3

/** Number of user parameters for controlling the filter.
 *
 * This is the number of cd_nelmts the user shall set at the call to
 * H5Pset_filter with the H5Z_FILTER_JPEGLS id.
 */
#define H5Z_FILTER_JPEGLS_USER_NPARAMS		(sizeof(jls_parameters_t)/sizeof(int))

/** Number of user parameters for controlling the filter.
 *
 * This is the number of cd_nelmts the user shall set at the call to
 * H5Pget_filter with the H5Z_FILTER_JPEGLS id.
 */
#define H5Z_FILTER_JPEGLS_NPARAMS			(1 + H5Z_FILTER_JPEGLS_MAX_NDIMS + H5Z_FILTER_JPEGLS_USER_NPARAMS)

/** Structure of the filter parameters.
 *
 * It contains only unsigned integers so that it may safely be casted
 * to an unsigned integer array cd_values[] at the input of the filter.
 */
typedef struct {
	unsigned int dataBytes; 						/**< data size (in bytes) */
	unsigned int dims[H5Z_FILTER_JPEGLS_MAX_NDIMS]; /**< dimension of the image: number of components, number of lines, number of columns. Fastest varying dimension last.*/
	jls_parameters_t jpeglsParameters; 				/**< user parameters for controlling the filter */
} jls_filter_parameters_t;


#endif /* H5ZJPEGLS_H_ */

