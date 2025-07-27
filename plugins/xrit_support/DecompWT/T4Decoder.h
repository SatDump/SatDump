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

#ifndef T4Decoder_included
#define T4Decoder_included

/*******************************************************************************

TYPE:
Concrete class.
      
PURPOSE:
Encapsulate the decoding (including error detection and recovery) of
T.4 encoded binary images.
      
FUNCTION:
class CT4Decoder:
	Holds the compressed & uncompressed buffer and performs the needed 
	operations to uncompress the compressed buffer, including error
	detection and recovery.
      
INTERFACES:
See 'INTERFACES' in the module declaration below
      
RESOURCES:	
Heap Memory (>2K).
      
REFERENCE:	
none

PROCESSING:	
The constructor takes a compressed image as parameter and allocates the
output image. If the image dimensions stated in the input buffer are
negative, the constructor tries to devise the image dimension by performing
a dummy decompression of the compressed image.
SkipToEOL() is a private function that scanns the compressed buffer till
it reaches an End Of Line marker, and positions the buffer index after
the EOL marker. It is used in the error recovery function.
SetLineValidity() is a private function, that is used to acces the 
m_QualityInfo valarray.
DecodeBuffer() is the public member function actually performing the 
decoding of the buffer.
GetDecompressedImage() public member function returning the uncompressed
image.
GetQualityInfo() public member function returning the quality info array.

DATA:
See 'Data' in the class header below.			

LOGIC:
-

*******************************************************************************/

#define NOMINMAX

#include <string>
#include <vector>
#include <memory>

#include "CDataField.h"
#include "CBitBuffer.h"
#include "T4Codes.h"

namespace COMP
{

	class CT4Decoder
	{

	private:
		// DATA:

		//	color constants.
		enum EColor
		{
			e_Black = false,
			e_White = true,
			e_lastColor
		};

		CT4Decodes m_AllDecodes; // the T4 codes & associated routines
		short m_height;			 // the size of the image
		short m_width;
		CBitBuffer m_ibuf;				  // the input (compressed) buffer
		std::unique_ptr<CBitBuffer> m_opbm; // the output (decompressed) buffer
		std::vector<short> m_QualityInfo; // the quality info vector

		// INTERFACE:

		// Description:	Scans the compressed buffer till a End Of Line marker is found.
		//				The buffer index is then positioned after the marker.
		// Returns:		Nothing.
		void SkipToEOL();

		// Description:	Set the validity information for the given line.
		// Returns:		Nothing.
		void SetLineValidity(
			short i_line,	 // line for which the validity information is to be set
			short i_nbpixels // number of valid pixels in the line. If the number of valid
							 // pixels is doubtful, this number is negative.
		)
		{
			COMP_TRYTHIS
			if (m_QualityInfo.size() > 0)
				m_QualityInfo[i_line] = i_nbpixels;
			COMP_CATCHTHIS
		}

	public:
		// Description:	Constructor. Initialise the decoder with its input data.
		//				If the image size provided in the input data are negative,
		//				the constructor tries to automatically determine the size of
		//				the compressed image. This is done by dummy-decoding the compressed buffer.
		// Returns:		Nothing.
		CT4Decoder(
			const Util::CDataFieldCompressedImage &i_datafield // compressed T.4 stream.
															   // Must have m_NB=1.
		);

		// Description:	Destructor.
		// Returns:		Nothing.
		~CT4Decoder()
		{
		}

		// Description:	Performs the actual T.4 decoding, including error detection & recovery.
		// Returns:		Nothing.
		void DecodeBuffer();

		// Description:	Return the decompressed image.
		// Returns:		The decompressed image, with m_NR =1
		Util::CDataFieldUncompressedImage GetDecompressedImage();

		// Description:	Returns the quality information array.
		// Returns:		A vector of one element per image line, each element containing
		//				the number of valid pixels in the corresponding line. A negative
		//				value indicates that this value is doubtful.
		std::vector<short> GetQualityInfo()
		{
			return m_QualityInfo;
		}
	};

} // end namespace

#endif
