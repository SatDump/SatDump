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

////
// \file
//
// Header file for the C API of the JPEG-LS facade.
//
// This includes the definition of the jls_parameters_t structure type
// and the facade to the CharLS software library.
//

#ifndef _FCICOMP_JPEGLS_H_
#define _FCICOMP_JPEGLS_H_


//// @{
// Control the JPEG-LS interleave mode parameter.
//
// ILV_NONE: colour components are provided at the input and compressed
// in band sequential order.
//
// ILV_LINE: compressor encodes one line at a time. Pixels are in band
// interleaved by line order.
//
// ILV_SAMPLE: compressor encodes a pixel at a time. For each pixel, the
// red, green and blue are sent immediately after each other. In this
// mode, only 3 component images are supported (so no RGBA). Pixels are
// in band interleaved by pixel order.
#define ILV_NONE   0
#define ILV_LINE   1
#define ILV_SAMPLE 2
// @}

//// @{
// Define JPEG-LS error codes.
//
// These are mainly error codes defined in CharLS API.
#define FJLS_NOERR									0
#define FJLS_INVALID_JPEGLS_PARAMETERS				1
#define FJLS_UNSUPPORTED_JPEGLS_PARAMETERS			2
#define	FJLS_UNCOMPRESSED_BUFER_TOO_SMALL			3
#define	FJLS_COMPRESSED_BUFER_TOO_SMALL				4
#define	FJLS_INVALID_COMPRESSED_DATA				5
#define	FJLS_TOO_MUCH_COMPRESSED_DATA				6
#define	FJLS_IMAGE_TYPE_NOT_SUPPORTED				7
#define FJLS_UNSUPPORTED_BITDEPTH_FOR_TRANSFORM		8
#define FJLS_UNSUPPORTED_COLOR_TRANSFORM			9
#define FJLS_MEMORY_ALLOCATION_ERROR				10
#define FJLS_UNKNOWN_ERROR							-1
// @}



// This is the structure of JPEG-LS preset coding parameters.
struct jls_preset_coding_parameters {
	unsigned int maxval;	// < Maximum possible value for any image sample
	unsigned int t1; 		// < First quantization threshold value for the local gradients
	unsigned int t2; 		// < Second quantization threshold value for the local gradients
	unsigned int t3; 		// < Third quantization threshold value for the local gradients
	unsigned int reset; 	// < Value at which the counters A, B, and N are halved
};


// This is the structure of JPEG-LS coding parameters.
typedef struct {
	unsigned int bit_per_sample;	// < Number of valid bits per sample to encode
	unsigned int components; 		// < Number of color components
	unsigned int ilv; 				// < Interleave mode in the compressed stream
	unsigned int near;				// < Difference bound for near-lossless coding
	struct jls_preset_coding_parameters preset; // < Structure of JPEG-LS preset coding parameters
} jls_parameters_t;

////
// @brief Compress an image in JPEG-LS.
//
// Compress the image in JPEG-LS using CharLS software library.
//
// Enough memory to hold the compressed image data should have been allocated
// by the calling function in the outBuf buffer.
//
// The compressedSize parameters holds the number of valid compressed bytes
// that have been written into the outBuf buffer.
//
// \param outBuf			output data buffer. Contains the compressed image
//                         	data.
// \param outBufSize		allocated memory for the output data buffer in
//                          bytes.
// \param compressedSize	effective size of the compressed data. This is
// 							the number of valid bytes in outBuf.
// \param inBuf				input image buffer.
// \param inBufSize			size of the input image buffer in bytes.
// \param samples			width of the image in pixels.
// \param lines				height of the image in pixels.
// \param jlsParams			JPEG-LS coding parameters structure.
// \return 0 in case of success, an error code > 0 otherwise.
int jpeglsCompress(void *outBuf, size_t outBufSize, size_t *compressedSize,
		const void *inBuf, size_t inBufSize, int samples, int lines,
		jls_parameters_t jlsParams);

////
// @brief Get the JPEG-LS coding parameters for a compressed image.
//
// Read the JPEG-LS coding parameters from the compressed image data using
// CharLS software library.
//
// The image parameters are returned into the samples, lines and jlsParams
// pointers.
//
// The jlsParams pointer may be NULL. In this case, only the compressed image
// dimensions are returned.
//
// \param inBuf		input compressed data buffer.
// \param inSize	size of the input compressed data buffer in bytes.
// \param samples	pointer on the width of the compressed image.
// \param lines		pointer on the height of the compressed image.
// \param jlsParams pointer on the JPEG-LS coding parameters structure.
// \return 0 in case of success, an error code > 0 otherwise.
//
int jpeglsReadHeader(const void *inBuf, size_t inSize, int *samples, int *lines,
		jls_parameters_t * jlsParams);

////
// @brief Decompress a JPEG-LS image.
//
// Decompress the JPEG-LS image using CharLS software library.
//
// Enough memory to hold the decompressed image data should have been allocated
// by the calling function in the outBuf buffer.
//
// \param outBuf	output data buffer. Contains the decompressed image data.
// \param outSize	allocated memory for the output data buffer in bytes.
// \param inBuf		input compressed data buffer.
// \param inSize	size of the input compressed data buffer in bytes.
// \return 0 in case of success, an error code > 0 otherwise.
//
int jpeglsDecompress(void *outBuf, size_t outSize, const void *inBuf,
		size_t inSize);

#endif // _FCICOMP_JPEGLS_H_
