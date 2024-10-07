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

 This file define the error messages printed by fcicomp_jpegls.

 */

#ifndef FCICOMP_JPEGLS_MESSAGES_H_
#define FCICOMP_JPEGLS_MESSAGES_H_

/* Error messages */
#define JPEGLS_COMPRESS_ERROR					"Error in jpeglsCompress: %s"
#define JPEGLS_READHEADER_ERROR					"Error in jpeglsReadHeader: %s"
#define JPEGLS_DECOMPRESS_ERROR					"Error in jpeglsDecompress: %s"
#define INVALID_JLS_PARAMETERS_MSG				"Parameter values are not a valid combination in JPEG-LS."
#define PARAMETER_VALUE_NOT_SUPPORTED_MSG		"Parameter values are not supported by CharLS."
#define UNCOMPRESSED_BUFFER_TOO_SMALL_MSG		"Not enough memory allocated for the output of the JPEG-LS decoding process."
#define COMPRESSED_BUFFER_TOO_SMALL_MSG			"Not enough memory allocated for the output of the JPEG-LS encoding process."
#define INVALID_COMPRESSED_DATA_MSG				"The compressed bit-stream cannot be decoded."
#define TOO_MUCH_COMPRESSED_DATA_MSG			"Too much compressed data."
#define IMAGE_TYPE_NOT_SUPPORTED_MSG			"The image type is not supported by CharLS."
#define UNKNOWN_CHARLS_ERROR_CODE_MSG			"Unknown CharLS error code."

/* Debug messages */
#define ENTER_FUNCTION							"-> Enter in %s()"
#define EXIT_FUNCTION							"<- Exit from %s() with code: %d"
#define CALL_CHARLS_JPEGLS_ENCODE				"-> Calling CharLS JpegLsEncode()"
#define EXIT_CHARLS_JPEGLS_ENCODE				"<- Exit from CharLS JpegLsEncode() with code: %d"
#define CALL_CHARLS_JPEGLS_READHEADER			"-> Calling CharLS JpegLsReadHeader()"
#define EXIT_CHARLS_JPEGLS_READHEADER			"<- Exit from CharLS JpegLsReadHeader() with code: %d"
#define CALL_CHARLS_JPEGLS_DECODE				"-> Calling CharLS JpegLsDecode()"
#define EXIT_CHARLS_JPEGLS_DECODE				"<- Exit from CharLS JpegLsDecode() with code: %d"

#define CHARLS_PARAMETERS						"CharLS parameters:\nheight:%d\nwidth:%d\nbitspersample:%d\ncomponents:%d\nilv:%d\nallowedlossyerror:%d\nMAXVAL:%d\nT1:%d\nT2:%d\nT3:%d\nRESET:%d"

#endif /* FCICOMP_JPEGLS_MESSAGES_H_ */
