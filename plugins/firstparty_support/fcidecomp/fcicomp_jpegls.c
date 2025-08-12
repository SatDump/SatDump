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
#include <string.h>

/* Includes from CharLS software */
#include "charls/charls.h"

/* The "fcicomjpegls.h" include should appear after
 * the include of "interface.h" of CharLS software
 * otherwise this provoke a compilation error. Why ? */
#include "fcicomp_jpegls.h"
#include "fcicomp_jpegls_messages.h"

/* Include from fcicomp-common */
#include "fcicomp_log.h"

/** Maximum number of components in the images. */
#define MAX_COMPONENTS	4

///* Convert error codes return by charls to error code returned by the fcicomp-jpegls module */
int charlsToFjlsErrorCode(int charlsErr) {


	/* Initialize the output error code */
	int fcicompJlsErr = FJLS_NOERR;

	/* Convert error codes return by charls */
	switch (charlsErr) {
	case CHARLS_JPEGLS_ERRC_SUCCESS:
		/* No error */
		fcicompJlsErr = FJLS_NOERR;
		break;
	case CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT:
		/* Parameter values are not a valid combination in JPEG-LS */
		fcicompJlsErr = FJLS_INVALID_JPEGLS_PARAMETERS;
		break;
	case CHARLS_JPEGLS_ERRC_PARAMETER_VALUE_NOT_SUPPORTED:
		/* Parameter values are not supported by CharLS */
		fcicompJlsErr = FJLS_UNSUPPORTED_JPEGLS_PARAMETERS;
		break;
	case CHARLS_JPEGLS_ERRC_DESTINATION_BUFFER_TOO_SMALL:
		/* Not enough memory allocated for the output of the JPEG-LS decode process */
		fcicompJlsErr = FJLS_UNCOMPRESSED_BUFER_TOO_SMALL;
		break;
	case CHARLS_JPEGLS_ERRC_SOURCE_BUFFER_TOO_SMALL:
		/* Not enough memory allocated for the output of the JPEG-LS encode process */
		fcicompJlsErr = FJLS_COMPRESSED_BUFER_TOO_SMALL;
		break;
	case CHARLS_JPEGLS_ERRC_INVALID_ENCODED_DATA:
		/* The compressed bitstream is not decodable */
		fcicompJlsErr = FJLS_INVALID_COMPRESSED_DATA;
		break;
	case CHARLS_JPEGLS_ERRC_TOO_MUCH_ENCODED_DATA:
		/* Too much compressed data */
		fcicompJlsErr = FJLS_TOO_MUCH_COMPRESSED_DATA;
		break;
	case CHARLS_JPEGLS_ERRC_INVALID_OPERATION:
		/* The image type used is not supported by CharLS */
		fcicompJlsErr = FJLS_IMAGE_TYPE_NOT_SUPPORTED;
		break;
	default:
		/* Default: Unknown CharLS error code */
		fcicompJlsErr = FJLS_UNKNOWN_ERROR;
		break;
	}

	/* Return the fcicomp-jpegls error code */
	return fcicompJlsErr;
}

///* Get the error messages corresponding to the input error code */
const char * getErrorMessage(int err) {


	/* Initialize the output message */
	char * msg = UNKNOWN_CHARLS_ERROR_CODE_MSG;

	/* Set the message string depending on the error */
	switch (err) {
	case CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT:
		/* Parameter values are not a valid combination in JPEG-LS */
		msg = INVALID_JLS_PARAMETERS_MSG;
		break;
	case CHARLS_JPEGLS_ERRC_PARAMETER_VALUE_NOT_SUPPORTED:
		/* Parameter values are not supported by CharLS */
		msg = PARAMETER_VALUE_NOT_SUPPORTED_MSG;
		break;
	case CHARLS_JPEGLS_ERRC_DESTINATION_BUFFER_TOO_SMALL:
		/* Not enough memory allocated for the output of the JPEG-LS decode process */
		msg = UNCOMPRESSED_BUFFER_TOO_SMALL_MSG;
		break;
	case CHARLS_JPEGLS_ERRC_SOURCE_BUFFER_TOO_SMALL:
		/* Not enough memory allocated for the output of the JPEG-LS encode process */
		msg = COMPRESSED_BUFFER_TOO_SMALL_MSG;
		break;
	case CHARLS_JPEGLS_ERRC_INVALID_ENCODED_DATA:
		/* The compressed bitstream is not decodable */
		msg = INVALID_COMPRESSED_DATA_MSG;
		break;
	case CHARLS_JPEGLS_ERRC_TOO_MUCH_ENCODED_DATA:
		/* Too much compressed data */
		msg = TOO_MUCH_COMPRESSED_DATA_MSG;
		break;
	case CHARLS_JPEGLS_ERRC_INVALID_OPERATION:
		/* The image type used is not supported by CharLS */
		msg = IMAGE_TYPE_NOT_SUPPORTED_MSG;
		break;
	default:
		/* Default: Unknown CharLS error code */
		msg = UNKNOWN_CHARLS_ERROR_CODE_MSG;
		break;
	}

	return msg;
}

///* Compress an image in JPEG-LS. */
int jpeglsCompress(void *outBuf, size_t outBufSize, size_t *compressedSize, const void *inBuf, size_t inBufSize,
		int samples, int lines, jls_parameters_t jlsParams) {
   
    
	LOG(DEBUG_SEVERITY, ENTER_FUNCTION, __func__);

	/* Initialize the output value */
	int result = FJLS_NOERR;

	/* Initialize the charls parameters structure with zeros.
	 * JlsParameters is the structure defined in charls software. */
	struct JlsParameters charlsParams = { 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 }, /* JlsCustomParameters */
	{ 0, 0, 0, 0, 0, 0, NULL } /* JfifParameters */
	};

	/* Check some of the input JPEG-LS parameters */
	if (jlsParams.components > MAX_COMPONENTS) {
		result = FJLS_INVALID_JPEGLS_PARAMETERS;
		LOG(ERROR_SEVERITY, JPEGLS_COMPRESS_ERROR, INVALID_JLS_PARAMETERS_MSG);

	} else {

		/* Fill the charls parameters structure */

		/* Set the image dimension in the charls parameters structure */
		charlsParams.height = lines;
		charlsParams.width = samples;

		/* Copy the data from the jls_parameters_t structure
		 * to the JlsParameters structure (specific to charls) */

		/* Number of valid bits per sample to encode */
		charlsParams.bitsPerSample = jlsParams.bit_per_sample;
		/* Number of colour components */
		charlsParams.components = jlsParams.components;
		/* Interleave mode in the compressed stream */
		charlsParams.interleaveMode = jlsParams.ilv;
		/* Difference bound for near-lossless coding */
		charlsParams.allowedLossyError = jlsParams.near;

		/* Structure of JPEG-LS coding parameters */
		/* Maximum possible value for any image sample */
		charlsParams.custom.MaximumSampleValue = jlsParams.preset.maxval;
		/* First quantization threshold value for the local gradients */
		charlsParams.custom.Threshold1 = jlsParams.preset.t1;
		/* Second quantization threshold value for the local gradients  */
		charlsParams.custom.Threshold2 = jlsParams.preset.t2;
		/* Third quantization threshold value for the local gradients  */
		charlsParams.custom.Threshold3 = jlsParams.preset.t3;
		/* Value at which the counters A, B, and N are halved  */
		charlsParams.custom.ResetValue = jlsParams.preset.reset;

		/* Initialize the return value for the call to CharLS */
		int charlsResult = CHARLS_JPEGLS_ERRC_SUCCESS;

		/* Encode the data using CharLS software */
		LOG(DEBUG_SEVERITY, CALL_CHARLS_JPEGLS_ENCODE);
		LOG(DEBUG_SEVERITY, CHARLS_PARAMETERS, charlsParams.height, charlsParams.width,
				charlsParams.bitsPerSample, charlsParams.components, charlsParams.interleaveMode, charlsParams.allowedLossyError,
				charlsParams.custom.MaximumSampleValue, charlsParams.custom.Threshold1, charlsParams.custom.Threshold2, charlsParams.custom.Threshold3, charlsParams.custom.ResetValue);
		charlsResult = JpegLsEncode(outBuf, outBufSize, compressedSize, inBuf, inBufSize, &charlsParams, NULL);
		LOG(DEBUG_SEVERITY, EXIT_CHARLS_JPEGLS_ENCODE, charlsResult);

		/* Check the result of JpegLsEncode */
		if (charlsResult != CHARLS_JPEGLS_ERRC_SUCCESS) {
			LOG(ERROR_SEVERITY, JPEGLS_COMPRESS_ERROR, getErrorMessage(charlsResult));
			/* Set the compressed size to 0 in case an error has occurred */
			*compressedSize = 0;
			/* Convert charls error code to fcicomp-jpegls error code */
			result = charlsToFjlsErrorCode(charlsResult);
		}
	}

	/* Return the error code */
	LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
	return result;
}

///* Get the JPEG-LS coding parameters for a compressed image. */
int jpeglsReadHeader(const void *inBuf, size_t inSize, int *samples, int *lines, jls_parameters_t * jlsParams) {


	LOG(DEBUG_SEVERITY, ENTER_FUNCTION, __func__);

	/* Initialize the output value */
	int result = FJLS_NOERR;

	/* Initialize the charls parameters structure with zeros.
	 * JlsParameters is the structure defined in charls software. */
	struct JlsParameters charlsParams = { 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 }, /* JlsCustomParameters */
	{ 0, 0, 0, 0, 0, 0, NULL } /* JfifParameters */
	};

	/* Initialize the return value for the call to CharLS */
	int charlsResult = CHARLS_JPEGLS_ERRC_SUCCESS;

	/* Read the JPEG-LS header using charls software */
	LOG(DEBUG_SEVERITY, CALL_CHARLS_JPEGLS_READHEADER);
	charlsResult = JpegLsReadHeader(inBuf, inSize, &charlsParams, NULL);
	LOG(DEBUG_SEVERITY, EXIT_CHARLS_JPEGLS_READHEADER, charlsResult);

	/* Check the result of JpegLsReadHeader */
	if (charlsResult == CHARLS_JPEGLS_ERRC_SUCCESS) {
		/* Get the image dimensions */
		*samples = charlsParams.width;
		*lines = charlsParams.height;

		/* The jlsParams pointer may be NULL.
		 * In this case, only the compressed image dimensions are returned. */
		if (jlsParams != NULL) {
			/* Copy the data from the JlsParameters structure (specific to charls)
			 * to the jls_parameters_t structure */

			/* Number of valid bits per sample to encode */
			jlsParams->bit_per_sample = charlsParams.bitsPerSample;
			/* Number of colour components */
			jlsParams->components = charlsParams.components;
			/* Interleave mode in the compressed stream */
			jlsParams->ilv = charlsParams.interleaveMode;
			/* Difference bound for near-lossless coding */
			jlsParams->near = charlsParams.allowedLossyError;

			/* Structure of JPEG-LS coding parameters */
			/* Maximum possible value for any image sample */
			jlsParams->preset.maxval = charlsParams.custom.MaximumSampleValue;
			/* First quantization threshold value for the local gradients */
			jlsParams->preset.t1 = charlsParams.custom.Threshold1;
			/* Second quantization threshold value for the local gradients  */
			jlsParams->preset.t2 = charlsParams.custom.Threshold2;
			/* Third quantization threshold value for the local gradients  */
			jlsParams->preset.t3 = charlsParams.custom.Threshold3;
			/* Value at which the counters A, B, and N are halved  */
			jlsParams->preset.reset = charlsParams.custom.ResetValue;
		}
	} else { /* charlsResult != CHARLS_JPEGLS_ERRC_SUCCESS */
		LOG(ERROR_SEVERITY, JPEGLS_READHEADER_ERROR, getErrorMessage(charlsResult));
		/* Convert charls error code to fcicomp-jpegls error code */
		result = charlsToFjlsErrorCode(charlsResult);
	}

	/* Return the error code */
	LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
	return result;
}

///* Decompress a JPEG-LS image. */
int jpeglsDecompress(void *outBuf, size_t outSize, const void *inBuf, size_t inSize) {


	LOG(DEBUG_SEVERITY, ENTER_FUNCTION, __func__);

	/* Initialize the output value */
	int result = FJLS_NOERR;

	/* Initialize the return value for the call to CharLS */
	int charlsResult = CHARLS_JPEGLS_ERRC_SUCCESS;

	/* Uncompress the JPEG-LS image using charls software */
	LOG(DEBUG_SEVERITY, CALL_CHARLS_JPEGLS_DECODE);
	charlsResult = JpegLsDecode(outBuf, outSize, inBuf, inSize, NULL, NULL);
	LOG(DEBUG_SEVERITY, EXIT_CHARLS_JPEGLS_DECODE, charlsResult);

	/* Check the result of JpegLsDecode */
	if (charlsResult != CHARLS_JPEGLS_ERRC_SUCCESS) {
		LOG(ERROR_SEVERITY, JPEGLS_DECOMPRESS_ERROR, getErrorMessage(charlsResult));
		/* Convert charls error code to fcicomp-jpegls error code */
		result = charlsToFjlsErrorCode(charlsResult);
	}

	/* Return the error code */
	LOG(DEBUG_SEVERITY, EXIT_FUNCTION, __func__, result);
	return result;
}

