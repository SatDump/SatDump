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

#ifndef CompressT4_included
#define CompressT4_included

/*******************************************************************************

TYPE:
Concrete classes and free functions.
					
PURPOSE:
Classes for the CCITT T.4 compression algorithm.
Free function for decompression.

FUNCTION:
Performs compression and decompression of image-type data according to the
CCITT T.4 coding scheme.

INTERFACES:
See 'INTERFACES:' in the module declaration below.

RESOURCES:	
Heap Memory (>2K).

REFERENCES:
None.

PROCESSING:
Derived from the abstract base class CCompress, the concrete class
- CCompressT4
provides the algorithm specific parameters and Compress() methods.

- DecompressT4() decompresses binary input image data which are coded according to
  the CCITT T.4 recommendation.

For passing image data to and from the routines, Util::CDataField... objects are
used.

DATA:
See 'DATA:' in the class header below.

LOGIC:		
-

*******************************************************************************/

#include <sstream>
#include <string>
#include <vector>
#include "CDataField.h"	   // Util
#include "SmartPtr.h"	   // Util
#include "ErrorHandling.h" // Util
#include "Compress.h"	   // COMP

namespace COMP
{

	// Compress() function for T4 compression.
	class CCompressT4 : public COMP::CCompress
	{

	public:
		// INTERFACES:

		// Description:	Compresses the input image data according to
		//				the specified parameters.
		// Returns:		Compressed data field.
		Util::CDataFieldCompressedImage Compress(
			const Util::CDataFieldUncompressedImage &i_Image // Image to compress.
		)
			const;

		// Description:	Constructor.
		// Returns:		Nothing.
		CCompressT4()
			: CCompress()
		{
		}

		// Description:	Tells whether compression is lossless or lossy.
		// Returns:		true : Compression is lossless.
		bool IsLossless()
			const
		{
			return true;
		}

		// Description:	Retruns the set of compresion parameters in string form.
		// Returns:		Compression parameters in readable form.
		std::string GetTraceString()
			const
		{
			return std::string("");
		}
	};

	// Description:	Decompresses binary input image data which are coded according to
	//				the CCITT T.4 recommendation.
	// Returns:		Nothing.
	void DecompressT4(
		const Util::CDataFieldCompressedImage &
			i_Image, // Compressed image data.
		Util::CDataFieldUncompressedImage &
			o_ImageDecompressed,		  // Decompressed image data.
		std::vector<short> &o_QualityInfo // Vector with one field per image line.
										  // The function will store there the number
										  // of 'good' pixels, counted from the first
										  // column.
										  // If there are no errors, the values will be
										  // equal to the total number of image columns.
	);

} // end namespace

#endif
