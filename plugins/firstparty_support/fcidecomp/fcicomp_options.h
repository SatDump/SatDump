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

 This file define the options and separators for fcicomp.

 */


#ifndef FCICOMP_OPTIONS
#define FCICOMP_OPTIONS

//SEPARATOR
#define FCI_SEPARATOR				 "/"
#define FCI_UNDERSCORE				 "_"
#define FCI_SHARP					 '#'
#define FCI_COMMA					 ","
#define FCI_POINT					 "."
#define FCI_PLUS						 "+"
#define FCI_EQUAL						 "="

#define FCI_DOT_CHAR				 '.'
#define FCI_SLASH_CHAR				 '/'

//COMMAND OPTIONS
#define FCI_HELP						 "-h"
#define FCI_HELP_FULL				 "--help"
#define FCI_HELP_U					 "-u"
#define FCI_INPUT					 "-i"
#define FCI_OUTPUT					 "-o"
#define FCI_PREFIX					 "-p"
#define FCI_NETCDF_CONFIG_FILE		 "-r"
#define FCI_JPEGLS_CONFIG_FILE		 "-j"
#define FCI_WIDTH					 "-w"
#define FCI_HEIGHT					 "-h"


//EXTENSIONS
#define FCI_RAW_EXT					 ".raw"
#define FCI_HDR_EXT					 ".hdr"
#define FCI_NC_EXT					 ".nc"
//FILE OPTIONS
#define FCI_WRITE					 "wb"
#define FCI_READ					 "rb"
#define FCI_READ_ONLY				 "r"

//OTHERS
#define FCI_QF						 "_qf"
#define FCI_BSQ						 "bsq"
#define FCI_SKIP_LINE				 " \t\r\n"

//NETCDF
#define FCI_0						 '\0'

//NUMBER
#define FCI_ONE						1
#define FCI_TWO						2
#define FCI_THREE					3
#define FCI_FOUR					4
#define FCI_FIVE					5
#define FCI_SIX						6
#define FCI_SEVEN					7
#define FCI_EIGHT					8
#define FCI_NINE					9
#define FCI_12						12
#define FCI_13						13
#define FCI_14						14
#define FCI_15						15
#define FCI_16						16
#define FCI_19						19
#define FCI_22						22
#define FCI_32						32
#define FCI_64						64

#define FCI_BYTE					8

#endif
