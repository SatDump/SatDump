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

#ifndef H5ZJPEGLS_DEFINITIONS_H_
#define H5ZJPEGLS_DEFINITIONS_H_

#include "H5Zjpegls.h"

/** Minimum number of pixels in the dataset */
#define MIN_PIXELS_NUMBER					16
/** Maximum spatial dimensions of the dataset */
#define MAX_XY_DIM							65535
/** Maximum number of components in the dataset.
 * Allow RGB, Alpha components */
#define MAX_Z_DIM							4

/** Margin factor on the compressed buffer size.
 *
 * This allows allocating slightly more memory than necessary for the cases
 * where the compression is not efficient e.g. noise image. Otherwise a
 * buffer overflow may occur in CharLS and also a potential seg fault.
 */
#define COMPRESSED_BUFFER_SIZE_MARGIN_FACTOR 1.2

/* VERSION:1.0.2:NCR:FCICOMP-12:04/09/2017:Add an offset on the allocated memory buffer for CharLS to prevent it crashes on very small images */

/** Offset on the compressed buffer size.
 *
 * This allows allocating memory that will hold the JPEG-LS header.
 */
#define COMPRESSED_BUFFER_SIZE_OFFSET		8086

///**
// * @brief Determine if the filter can be applied.
// *
// * Determine whether the combination of the dataset creation property
// * list values, the datatype, and the dataspace represent a valid
// * combination to apply this filter to.
// *
// * @param dcpl_id   dataset's dataset creation property list,
// * @param type_id   dataset's datatype
// * @param space_id  dataspace describing a chunk (for chunked dataset storage)
// * @return a positive value for a valid combination, zero for an invalid combination, and a negative value for an error.
// */
htri_t can_apply(hid_t dcpl_id, hid_t type_id, hid_t space_id);

///**
// * @brief Set any parameters that are specific to this dataset.
// *
// * Set any parameters that are specific to this dataset, based on the
// * combination of the dataset creation property list values, the datatype,
// * and the dataspace.
// *
// * @param dcpl_id   dataset's private copy of the dataset creation property list passed in to H5Dcreate (i.e. not the actual property list passed in to H5Dcreate).
// * @param type_id   datatype identifier passed in to H5Dcreate, which is not copied and should not be modified.
// * @param space_id  dataspace describing the chunk (for chunked dataset storage), which should also not be modified.
// * @return a non-negative value on success and a negative value for an error.
// */
herr_t set_local(hid_t dcpl_id, hid_t type_id, hid_t space_id);

///**
// * @brief Performs the action of the filter i.e. either the JPEG-LS
// * compression or decompression of the input data.
// *
// * This function calls either the H5Z_filter_jpegls_encode function or the
// * H5Z_filter_jpegls_decode function based on the filter direction flag.
// *
// * @param flags bit  vector specifying certain general properties of the filter (e.g. H5Z_FLAG_REVERSE).
// * @param cd_nelmts  size of the cd_values array.
// * @param cd_values  array of cd_nelmts integers which are auxiliary data for the filter. The auxiliary data are defined by the structure jls_filter_parameters_t.
// * @param nBytes     number of bytes of valid data in the input buffer buf.
// * @param buf_size   size of the input buffer buf.
// * @param buf        pointer on the input buffer which has a size of *buf_size bytes, nbytes of which are valid data.
// * @return the number of valid bytes of data contained in buf if successful, 0 otherwise.
// */
size_t H5Z_filter_jpegls(unsigned int flags, size_t cd_nelmts,
		const unsigned int cd_values[], size_t nbytes, size_t *buf_size,
		void **buf);

///**
// * @brief Performs the action of the filter i.e. the JPEG-LS compression of the input data.
// *
// * @param cd_nelmts  size of the cd_values array.
// * @param cd_values  array of cd_nelmts integers which are auxiliary data for the filter. The auxiliary data are defined by the structure jls_filter_parameters_t.
// * @param nBytes     number of bytes of valid data in the input buffer buf.
// * @param buf_size   size of the input buffer buf.
// * @param buf        pointer on the input buffer which has a size of *buf_size bytes, nbytes of which are valid data.
// * @return the number of valid bytes of data contained in buf if successful, 0 otherwise.
// */
size_t H5Z_filter_jpegls_encode(size_t cd_nelmts,
		const unsigned int cd_values[], size_t nbytes, size_t *buf_size,
		void **buf);

///**
// * @brief Performs the action of the filter i.e. the JPEG-LS decompression of the input data.
// *
// * @param flags bit  vector specifying certain general properties of the filter (e.g. H5Z_FLAG_REVERSE).
// * @param cd_nelmts  size of the cd_values array.
// * @param cd_values  array of cd_nelmts integers which are auxiliary data for the filter.
// * @param nBytes     number of bytes of valid data in the input buffer buf.
// * @param buf_size   size of the input buffer buf.
// * @param buf        pointer on the input buffer which has a size of *buf_size bytes, nbytes of which are valid data.
// * @return the number of valid bytes of data contained in buf if successful, 0 otherwise.
// */
size_t H5Z_filter_jpegls_decode(size_t cd_nelmts,
		const unsigned int cd_values[], size_t nbytes, size_t *buf_size,
		void **buf);

#endif /* H5ZJPEGLS_DEFINITIONS_H_ */

