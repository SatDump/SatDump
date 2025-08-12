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

 This file define the errors message of fcicomp.

 */

#ifndef FCICOMP_ERRORS
#define FCICOMP_ERRORS

static int ERR_TEST(int e, char * msg) {fputs((msg), stderr); return (e);}

//#define ERR_TEST(e, msg) {fputs((msg), stderr); return (e);}

#define INVALID_NUMBER_ARGUMENTS 			  	"Invalid number of arguments !\n"
#define TOO_MANY_ARGUMENTS 				  	"Too many input arguments.\n"
#define MISSING_INPUT_ARGUMENTS 			  	"Missing input arguments.\n"
#define UNKNOWN_ARGUMENT 					  	"Unknown argument %s.\n"
#define UNEXPECTED_FILTER					 	"Unexpected filter!\n"
#define MEMORY_ALLOCATION_ERROR			 	"Memory allocation error!\n"
#define CANNOT_OPEN_FILE_W					 	"Cannot open file for writing!\n"
#define ERROR_WRITING_FILE					 	"Error writing file!\n"
#define ERROR_DURING_COMPRESSION			 	"Error during the compression!\n"
#define ERROR_DURING_DECOMPRESSION			 	"Error during the decompression!\n"
#define CANNOT_OPEN_FILE_R					 	"Cannot open file for reading!\n"
#define ERROR_READING_FILE					 	"Error reading file!\n"
#define JPEG_LS_FILTER_UNVAILABLE			 	"JPEG-LS encoding filter not available!\n"
#define ERROR_READING_JPEGLS_HEADER		 	"Error reading the JPEG-LS header!\n"
#define TRANSPARENT_FILTER_UNVAILABLE		 	"Transparent filter not available!\n"
#define TEST_NOT_DEFINED					 	"Test %s is not defined.\n"
#define CANNOT_READ_DATA					 	"Cannot read the image data file %s\n"


#endif
