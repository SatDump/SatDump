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

 This file define the error messages printed by H5Zjpegls.

 */

#ifndef H5ZJPEGLS_MESSAGES_H_
#define H5ZJPEGLS_MESSAGES_H_

/* Error messages */
#define JPEGLS_COMPRESSION_ERROR 			"Error during the JPEG-LS compression of the dataset."
#define JPEGLS_DECOMPRESSION_ERROR 			"Error during the JPEG-LS decompression of the dataset."
//#define JPEGLS_READ_HEADER_ERROR 			 "Error reading JPEG-LS header from dataset."
#define COMPRESSED_BUFFER_OVERFLOW_ERROR	"Buffer overflow. The compressed size is larger than the memory allocated to hold the compressed data! The allocated memory for the compressed buffer cannot be freed!"
#define WRONG_FILTER_DIRECTION				"Wrong filter direction in the HDF5 JPEG-LS filter!"

#define INVALID_DATASPACE_MSG				"Invalid HDF5 data space. Data space must be simple to be able to apply JPEG-LS filter."
#define INVALID_DATATYPE_MSG				"Invalid HDF5 data type. Data type must be integers to be able to apply JPEG-LS filter."
#define INVALID_DATABYTES_MSG				"Invalid number of bytes per sample. Data must be on one or two bytes per samples to be able to apply JPEG-LS filter."
#define INVALID_BYTEORDER_MSG				"Invalid byte order. Data must be either in big or little-endian to be able to apply JPEG-LS filter."
#define INVALID_NUMBER_OF_DIMENSIONS_MSG	"Invalid number of dimensions. Data must have 2 (or 3 dimensions in the case of color images) to be able to apply JPEG-LS filter."
#define INVALID_NUMBER_OF_COMPONENTS_MSG	"Invalid number of components. Data must have between 1 and 4 color components to be able to apply JPEG-LS filter."
#define INVALID_DIMENSIONS_MSG				"Invalid dimensions. Too few pixels or dimensions too large to be able to apply JPEG-LS filter."
#define INVALID_NUMBER_OF_PARAMETERS_MSG	"Invalid number of parameters in the HDF5 JPEG-LS filter."
#define INVALID_BUFFER_SIZE_MSG				"Invalid number of bytes passed at the input of the HDF5 JPEG-LS filter: The input number of bytes does not correspond to the size of the dataset."
#define INVALID_NUMBER_OF_BYTES_MSG			"Invalid buffer size passed at the input of the HDF5 JPEG-LS filter: The input buffer size is smaller than the size of the dataset."


/* Warning messages */
#define INEFFICIENT_COMPRESSION_MSG			"HDF5 JPEG-LS compression filter is not efficient on this dataset: The compressed size is larger than the uncompressed size! Data are let uncompressed."
// #define FILTER_NOT_AVAILABLE_MSG 			"HDF5 JPEG-LS compression filter is not available."
#define FAIL_TO_GET_FILTER_PARAMS_MSG		"HDF5 JPEG-LS filter failed to get user defined JPEG-LS compression parameters. Setting default JPEG-LS compression parameters."

/* Debug messages */
#define ENTER_FUNCTION						"-> Enter in %s()"
#define EXIT_FUNCTION						"<- Exit from %s() with code: %d"
#define CALL_JPEGLS_COMPRESS	      		"-> Calling jpeglsCompress"
#define EXIT_JPEGLS_COMPRESS	    		"<- Exit from jpeglsCompress with code: %d"
#define CALL_H5P_MODIFY_FILTER	      		"-> Calling H5Pmodify_filter"
#define EXIT_H5P_MODIFY_FILTER	    		"<- Exit from H5Pmodify_filter with code: %d"

#define CALL_JPEGLS_DECOMPRESS	      		"-> Calling jpeglsDecompress"
#define EXIT_JPEGLS_DECOMPRESS	    		"<- Exit from jpeglsDecompress with code: %d"

#endif /* H5ZJPEGLS_MESSAGES_H_ */
