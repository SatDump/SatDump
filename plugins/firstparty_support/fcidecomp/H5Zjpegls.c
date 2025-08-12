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

#include <stdlib.h>
#include <H5PLextern.h>
#include "fcicomp_jpegls.h"

#include "H5Zjpegls.h"
#include "H5Zjpegls_definitions.h"
#include "H5Zjpegls_messages.h"

/* Include from fcicomp-common */
#include "fcicomp_log.h"
#include "fcicomp_options.h"
#include "fcicomp_errors.h"

/** Registers the JPEG-LS coding filter. */
const H5Z_class2_t H5Z_JPEGLS[1] = { {
H5Z_CLASS_T_VERS, /**< H5Z_class_t version */
(H5Z_filter_t) H5Z_FILTER_JPEGLS, /**< Filter id number */
1, /**< encoder_present flag (set to true) */
1, /**< decoder_present flag (set to true) */
"HDF5 JPEG-LS filter", /**< Filter name */
can_apply, /**< The "can apply" callback     */
set_local, /**< The "set local" callback     */
(H5Z_func_t) H5Z_filter_jpegls, /**< The actual filter function   */
} };

#if 0
// Get plugin type
H5PL_type_t H5PLget_plugin_type(void) {
	return H5PL_TYPE_FILTER;
}
// Get plugin info
const void *H5PLget_plugin_info(void) {
	return H5Z_JPEGLS;
}
#endif

void register_MTG_FILTER() {
	herr_t status = H5Zregister(H5Z_JPEGLS);
}

///* Determine if the filter can be applied */
htri_t can_apply(hid_t dcpl_id, hid_t type_id, hid_t space_id) {
	LOG(DEBUG_SEVERITY, ENTER_FUNCTION, __func__);

	/* Initialize the exit status to false */
	htri_t result = 0;

	/* The following block of lines could be commented because this can never happen.
	 * Indeed filters can only be used with chunked layout and dataspace is
	 * necessarily simple for chunks to be defined. But they are let
	 * uncommented for the compiler not to shout because it is the only
	 * instruction using the space_id variable. */

	/* The data space must be simple */
	if (H5Sis_simple(space_id) <= 0) {
		/* Exit failure: the filter cannot be applied */
		LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
		ERR(result, INVALID_DATASPACE_MSG);
	}

	/* The data type class must be integer */
	if (H5Tget_class(type_id) != H5T_INTEGER) {
		/* Exit failure: the filter cannot be applied */
		LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
		ERR(result, INVALID_DATATYPE_MSG);
	}

	/* The data must be one 1 or 2 bytes */
	size_t data_bytes = H5Tget_size(type_id);
	if ((data_bytes != 1) && (data_bytes != FCI_TWO)) {
		/* Exit failure: the filter cannot be applied */
		LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
		ERR(result, INVALID_DATABYTES_MSG);
	}

	/* The data must be either big or little-endian */
	H5T_order_t byte_order = H5Tget_order(type_id);
	if ((byte_order != H5T_ORDER_LE) && (byte_order != H5T_ORDER_BE)
			&& (byte_order != H5T_ORDER_NONE)) {
		/* Exit failure: the filter cannot be applied */
		LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
		ERR(result, INVALID_BYTEORDER_MSG);
	}

	/* Get the chunk dimensions */
	hsize_t dims[H5Z_FILTER_JPEGLS_MAX_NDIMS] = { 0 };
	int rank = H5Pget_chunk(dcpl_id, H5Z_FILTER_JPEGLS_MAX_NDIMS, dims);
	/* The number of dimensions of the dataset must be 2 or 3 */
	if ((rank != FCI_TWO) && (rank != FCI_THREE)) {
		/* Exit failure: the filter cannot be applied */
		LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
		ERR(result, INVALID_NUMBER_OF_DIMENSIONS_MSG);
	}

	/* Initialize the spatial dimensions variables */
	unsigned int samples = dims[FCI_TWO];
	unsigned int lines = dims[FCI_ONE];

	if (rank == FCI_THREE) {
		/* The number of components must be greater or equal to 1 and smaller or equal to MAX_Z_DIM */
		if ((dims[0] < 1) || (dims[0] > MAX_Z_DIM)) {
			/* Exit failure: the filter cannot be applied */
			LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
			ERR(result, INVALID_NUMBER_OF_COMPONENTS_MSG);
		}
		/* Get the spatial dimensions in the color image case (multiple-components)
		 * Remember, convention is fastest varying dimension last. */
		samples = dims[FCI_TWO];
		lines = dims[FCI_ONE];
	} else { /* rank == 2 */
		/* Get the spatial dimensions in the gray image case (single-components)
		 * Remember, convention is fastest varying dimension last. */
		samples = dims[FCI_ONE];
		lines = dims[0];
	}

	/* Check the XY dimensions: the number of pixels shall not be too small
	 * and the dimensions of the image shall not be too large */
	if ((samples * lines < MIN_PIXELS_NUMBER) || (samples > MAX_XY_DIM)
			|| (lines > MAX_XY_DIM)) {
		/* Exit failure: the filter cannot be applied */
		LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
		ERR(result, INVALID_DIMENSIONS_MSG);
	}

	/* Exit success: the filter can be applied */
	result = 1;
	LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
	return result;
}

///* Set any filter parameters that are specific to this dataset */
herr_t set_local(hid_t dcpl_id, hid_t type_id, hid_t space_id) {
	LOG(DEBUG_SEVERITY, ENTER_FUNCTION, __func__);

	/* Initialize the exit status to false */
	htri_t result = 0;

	/* Create the parameters structure that point to the cd_value array */
	jls_filter_parameters_t params = { 0, /* dataBytes*/
	{ 0, 0, 0 }, /* dims,  WARNING: Depends on H5Z_FILTER_JPEGLS_MAX_NDIMS */
	{ 0, 0, 0, 0, { 0, 0, 0, 0, 0 } } /* jpeglsParameters */
	};

	/* Get the size of the data type */
	params.dataBytes = (unsigned int) H5Tget_size(type_id);

	/* Initialize the output variable for the call to H5Pget_filter_by_id */
	unsigned int flags = 0;
	unsigned int filter_config = 0;
	/* User should have set this number of parameters when calling the filter */
	size_t cd_nelmts = H5Z_FILTER_JPEGLS_USER_NPARAMS;
	/* Make the cd_values array points on the jpeglsParameters structure to
	 * retrieve the parameters provided by the user.*/
	unsigned int * cd_values = (unsigned int *) &(params.jpeglsParameters);
	/* Retrieve the filters parameters that have been set by the user */
	if (H5Pget_filter_by_id(dcpl_id, H5Z_FILTER_JPEGLS, &flags, &cd_nelmts,
			cd_values, 0, NULL, &filter_config) < 0) {
		LOG(WARNING_SEVERITY, FAIL_TO_GET_FILTER_PARAMS_MSG);
	}

	if (params.jpeglsParameters.bit_per_sample == 0) {
		/* If the user has not set the number of bit per samples,
		 * set the number of bits per sample corresponding to the number of
		 * bytes per samples in the input dataset */
		params.jpeglsParameters.bit_per_sample = FCI_BYTE * params.dataBytes;
	}

	/* Get the rank of the dataset */
	int rank = H5Sget_simple_extent_ndims(space_id);

	/* Get the chunk dimensions */
	hsize_t dims[H5Z_FILTER_JPEGLS_MAX_NDIMS] = { 0 };
	H5Pget_chunk(dcpl_id, H5Z_FILTER_JPEGLS_MAX_NDIMS, dims);
	/* Set the spatial dimensions and the number of color components */
	switch (rank) {
	case FCI_TWO:
		/* Permute the dimension to set the number of color component to 1.
		 * Remember, convention is fastest varying dimension last. */
		dims[FCI_TWO] = dims[FCI_ONE];
		dims[FCI_ONE] = dims[0];
		dims[0] = 1;
		break;
	case FCI_THREE:
		break;
	default:
		LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
		ERR(result, INVALID_NUMBER_OF_DIMENSIONS_MSG);
	}

	/* Copy the dimensions into the filter parameters structure */
	unsigned int i = 0;
	for (i = 0; i < H5Z_FILTER_JPEGLS_MAX_NDIMS; i++)
		params.dims[i] = (unsigned int) dims[i];

	/* Set the number of components in the JPEG-LS parameters structure */
	params.jpeglsParameters.components = params.dims[0];

	/* Make the cd_values array points at the beginning of the filter
	 * parameters structure, and then pass it to the filter by calling H5Pmodify_filter */
	cd_values = (unsigned int *) (&params);
	/* Modify the filter to apply the parameters we just got and force it to optional:
	 * if the JPEG-LS compression fails, we keep the data uncompressed.*/
	LOG(DEBUG_SEVERITY, CALL_H5P_MODIFY_FILTER);
	result = H5Pmodify_filter(dcpl_id, H5Z_FILTER_JPEGLS,
	H5Z_FLAG_OPTIONAL, H5Z_FILTER_JPEGLS_NPARAMS, cd_values);
	LOG(DEBUG_SEVERITY, EXIT_H5P_MODIFY_FILTER, result);

	/* Exit with the result of the H5Pmodify_filter command */
	LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
	return result;
}

///* Select the JPEG-LS compression or decompression function */
size_t H5Z_filter_jpegls(unsigned int flags, size_t cd_nelmts,
		const unsigned int cd_values[], size_t nbytes, size_t *buf_size,
		void **buf) {

	/* Initialize the return value */
	size_t result = 0;

	/* Check the filter direction flag */
	if (flags & H5Z_FLAG_REVERSE) {
		/* Call the decode function */
		result = H5Z_filter_jpegls_decode(cd_nelmts, cd_values, nbytes,
				buf_size, buf);
	} else {
		/* Call the encode function */
		result = H5Z_filter_jpegls_encode(cd_nelmts, cd_values, nbytes,
				buf_size, buf);
	}

	return result;
}

///* Perform the JPEG-LS compression */
size_t H5Z_filter_jpegls_encode(size_t cd_nelmts,
		const unsigned int cd_values[], size_t nbytes, size_t *buf_size,
		void **buf) {
	LOG(DEBUG_SEVERITY, ENTER_FUNCTION, __func__);


	/* Initialize the return value */
	size_t result = 0;

	/* Compute the number of parameters in the filter parameters structure */
	size_t n_params = sizeof(jls_filter_parameters_t) / sizeof(int);

	/* Check the number of input parameters */
	if (cd_nelmts != n_params) {
		/* Return failure! */
		ERR(result, INVALID_NUMBER_OF_PARAMETERS_MSG);
	}

	/* Fill the structure with the input parameters */
	jls_filter_parameters_t * params = (jls_filter_parameters_t *) cd_values;

	/* Get the dataset dimensions.
	 * Remember, convention is fastest varying dimension last. */
	unsigned int components = params->dims[0];
	unsigned int lines = params->dims[FCI_ONE];
	unsigned int samples = params->dims[FCI_TWO];

	/* Do not compress images with a wrong number of components */
	/* The number of components must be greater or equal to 1 and smaller or equal to MAX_Z_DIM */
	if ((components < 1) || (components > MAX_Z_DIM)) {
		/* Exit failure! */
		LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
		ERR(result, INVALID_NUMBER_OF_COMPONENTS_MSG);
	}

	/* Compute the number of pixels in one component */
	size_t pixelsPerComponent = samples * lines;
	/* Do not compress too small images */
	if (pixelsPerComponent < MIN_PIXELS_NUMBER) {
		/* Return failure! */
		LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
		ERR(result, INVALID_DIMENSIONS_MSG);
	}
	/* Compute the number of pixels in the dataset */
	unsigned long pixels = pixelsPerComponent * components;
	/* Do not compress images with a wrong number of bytes */
	if ((params->dataBytes != 1) && (params->dataBytes != FCI_TWO)) {
		/* Return failure! */
		LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
		ERR(result, INVALID_DATABYTES_MSG);
	}
	/* Compute the uncompressed size of the input buffer */
	size_t uncompressedBufferSize = pixels * params->dataBytes;
	/* Check that the input number of valid bytes is correct */
	if (nbytes != uncompressedBufferSize) {
		/* Return failure! */
		LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
		ERR(result, INVALID_NUMBER_OF_BYTES_MSG);
	}
	/* Check that the size of the input buffer is correct */
	if (*buf_size < uncompressedBufferSize) {
		/* Return failure! */
		LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
		ERR(result, INVALID_BUFFER_SIZE_MSG);
	}

	/* Create the JPEG-LS parameters structure */
	jls_parameters_t jlsParams = params->jpeglsParameters;

	/* Allocate memory to hold the compressed data.
	 * Allocate slightly more memory than the input data size to be able
	 * to hold JPEG-LS header even if the compression ratio is slightly lower than as 1.0! */

	/* VERSION:1.0.2:NCR:FCICOMP-12:04/09/2017:Add an offset on the allocated
	 * memory buffer for CharLS to prevent it segfault on very small images */
	size_t compressedBufSize =
			(size_t) ((float) COMPRESSED_BUFFER_SIZE_MARGIN_FACTOR * nbytes + COMPRESSED_BUFFER_SIZE_OFFSET);

	void * compressedBuf = malloc(compressedBufSize);
	if (compressedBuf == NULL) {
		/* Return failure! */
		LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
		ERR(result, MEMORY_ALLOCATION_ERROR);
	}

	/* Initialize the compressed size variable (in bytes) */
	size_t compressedSize = 0;

	/* Compress the dataset using JPEG-LS */
	LOG(DEBUG_SEVERITY, CALL_JPEGLS_COMPRESS);
	int jlsResult = jpeglsCompress(compressedBuf, compressedBufSize,
			&compressedSize, *buf, nbytes, samples, lines, jlsParams);
	LOG(DEBUG_SEVERITY, EXIT_JPEGLS_COMPRESS, jlsResult);

	if (jlsResult == FJLS_NOERR) {

		/* JPEG-LS compression success */

		if (compressedSize < nbytes) {

			/* Regular case:
			 * If the compressed data is smaller than the uncompressed data size,
			 * replace the input/output buffer by the compressed buffer
			 * and return the compressed data size.*/
			free(*buf);
			*buf = compressedBuf;
			*buf_size = compressedBufSize;
			/* Return the number of compressed bytes */
			result = compressedSize;

		} else {

			/* Otherwise, if the compressed data is larger than the uncompressed data,
			 * let the buffer unchanged and return the uncompressed data size */
			LOG(WARNING_SEVERITY, INEFFICIENT_COMPRESSION_MSG);
			/* Return failure! */
			result = 0;

			if (compressedSize > compressedBufSize) {
				/* If the compressed size is larger than the compressed buffer size,
				 * we are in the case of a buffer overflow.
				 * We cannot free the compressedBuf otherwise a
				 * "*** glibc detected *** [...] free(): invalid next size (normal)"
				 * error is generated. */
				LOG(ERROR_SEVERITY, COMPRESSED_BUFFER_OVERFLOW_ERROR);
			} else {
				free(compressedBuf);
			}
		}
	} else {

		/* JPEG-LS compression failed */
		LOG(ERROR_SEVERITY, JPEGLS_COMPRESSION_ERROR);
		/* Return failure! */
		result = 0;
	}

	/* Return with the exit value */
	LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
	return result;
}

///* Perform the JPEG-LS decompression */
size_t H5Z_filter_jpegls_decode(size_t cd_nelmts,
		const unsigned int cd_values[], size_t nbytes, size_t *buf_size,
		void **buf) {
	LOG(DEBUG_SEVERITY, ENTER_FUNCTION, __func__);


	/* Initialize the return value */
	size_t result = 0;

	/* Initialize the decoded data buffer */
	void * decodedBuf = NULL;

	/* ------------------------------------------------------
	 * Get the dataset dimensions from the filter parameters
	 * ------------------------------------------------------ */

	/* Check the number parameters in the filter */
	if (cd_nelmts == H5Z_FILTER_JPEGLS_NPARAMS) {

		/* Get the filter parameters */
		jls_filter_parameters_t * params = (jls_filter_parameters_t *) cd_values;

		/* Get the dataset dimensions */
		unsigned int components = params->dims[0];
		unsigned int lines = params->dims[FCI_ONE];
		unsigned int samples = params->dims[FCI_TWO];

		/* Get the number of bytes per samples in the uncompressed dataset */
		unsigned int dataBytes = params->dataBytes;

		/* Compute the size of the decoded dataset */
		size_t decodedSize = components * lines * samples * dataBytes;

		/* Allocate memory for the decoded image */
		decodedBuf = malloc(decodedSize);
		if (decodedBuf != NULL) {

			/* ------------------------------------------
			 * Perform JPEG-LS decompression
			 * ------------------------------------------ */

			/* Decompress */
			LOG(DEBUG_SEVERITY, CALL_JPEGLS_DECOMPRESS);
			int jlsResult = jpeglsDecompress(decodedBuf, decodedSize, *buf,
					nbytes);
			LOG(DEBUG_SEVERITY, EXIT_JPEGLS_DECOMPRESS, jlsResult);

			if (jlsResult == FJLS_NOERR) {

				/* JPEG-LS decompression success */

				/* Replace the input/output buffer by the decoded buffer
				 * and return the compressed data size.*/
				free(*buf);
				*buf = decodedBuf;
				/* Set decodedBuf to NULL because otherwise the buffer is
				 * freed at the end of the function */
				decodedBuf = NULL;
				*buf_size = decodedSize;

				/* Return the number of decoded bytes */
				result = decodedSize;

			} else { /* JPEG-LS decompression has failed */
				/* Return failure!*/
				LOG(ERROR_SEVERITY, JPEGLS_DECOMPRESSION_ERROR);
			}

		} else { /* Memory allocation error */
			/* Return failure!*/
			LOG(ERROR_SEVERITY, MEMORY_ALLOCATION_ERROR);
		}

	} else { /* Invalid number of parameters in the filter */
		/* Return failure!*/
		LOG(ERROR_SEVERITY, INVALID_NUMBER_OF_PARAMETERS_MSG);
	}

	/* Free the allocated memory if not already done */
	if (decodedBuf != NULL) {
		free(decodedBuf);
	}

	/* Return with the exit value */
	LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
	return result;
}

