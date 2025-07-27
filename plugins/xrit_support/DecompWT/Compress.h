/*
 * Copyright 2011-2019, European Organisation for the Exploitation of Meteorological Satellites (EUMETSAT)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef Compress_included
#define Compress_included

/*******************************************************************************

TYPE:
Template Class.
					
PURPOSE:
Provides an abstract base class for a compression algorithm.

FUNCTION:
Provides an abstract base class for a compression algorithm.

INTERFACES:
See 'INTERFACES:' in the module declaration below.

RESOURCES:	
None.

REFERENCES:
None.

PROCESSING:
None.
For passing image data to and from the routines, Util::CDataField... objects are
used.

DATA:
See 'DATA:' in the class header below.

LOGIC:		
-

*******************************************************************************/

#include <string>

#include "CDataField.h" // Util
#include "SmartPtr.h"	// Util

namespace COMP
{

	// Enumeration of the requantization modes.
	enum ERequantizationMode
	{
		e_NoRequantization = 0,
		e_10to12 = 1,		// 10bpp to 12bpp padding
		e_10to8_floor = 2,	// 10bpp to 8bpp conversion by           floor(x  ).
		e_10to8_floor1 = 3, // 10bpp to 8bpp conversion by           floor(x+1).
		e_10to8_round = 4,	// 10bpp to 8bpp conversion by rounding (floor(x+2)).
		e_10to8_ceil = 5	// 10bpp to 8bpp conversion by ceiling  (floor(x+3)).
	};

	// Abstract base class.
	class CCompress
	{

	public:
		// INTERFACES:

		// Description:	Destructor.
		// Returns:		Nothing.
		virtual ~CCompress()
		{
		}

		// Description:	Compresses the input image data according to
		//				the specified parameters.
		// Returns:		Compressed data field.
		virtual Util::CDataFieldCompressedImage Compress(
			const Util::CDataFieldUncompressedImage &i_Image // Image to compress.
		)
			const = 0;

		// Description:	Tells whether compression is lossless or lossy.
		// Returns:		true : Compression is lossless.
		//				false: Compression is lossy.
		virtual bool IsLossless()
			const = 0;

		// Description:	Returns the set of compression parameters in string form.
		// Returns:		Compression parameters in readable form.
		virtual std::string GetTraceString()
			const = 0;
	};

} // end namespace

#endif
