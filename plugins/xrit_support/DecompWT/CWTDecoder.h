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

#ifndef CWTDecoder_included
#define CWTDecoder_included

/*******************************************************************************

TYPE:
Concrete class.

PURPOSE: 
Base Wavelet Decoder object featuring common functionalities between all possible
decoders.

FUNCTION:	
This object is merely a container for common Wavelet-decoder sub-objects
and is used to have a centralised initialisation point.

INTERFACES:	
See 'INTERFACES' in the module declaration below 

RESOURCES:	
Heap Memory (>2K).

REFERENCE:	
None.

PROCESSING:	
The constructor takes a CDataFielCompressed image and uses it to initialise the
subobjects.
The DecodeBuffer member function starts the actual decoding.
The GetDecompressedImage function returns the CDataFieldUncompressedImage after coding.
The GetQualityInfo function allows to retrieve the quality information.

DATA:
See 'DATA' in the module declaration below 

LOGIC:
				
*******************************************************************************/

#include "CDataField.h"
#include "CompressWT.h"
#include "CImage.h"
#include "CBuffer.h"
#include "CQualityInfo.h"

namespace COMP
{

	class CWTDecoder
	{

	private:
		// DATA:

		CImage m_Image;				 //	image data buffer
		CWTParams m_Param;			 //	compression parameters
		CRBuffer m_Buf;				 //	compressed data buffer
		CQualityInfo m_Qinfo;		 //	quality information vector
		unsigned int m_LastGoodLine; //  last line decoded without error

		// INTERFACES:

		// Description:	Performs the decoding of the input buffer by block.
		// Returns:		False if decoding failed, true otherwise.
		bool DecodeBufferBlock(
			const unsigned int i_Bs // block size
		);

		// Description:	Performs the coding of the input buffer as a whole.
		// Returns:		False if decoding failed, true otherwise.
		bool DecodeBufferFull();

		// Description:	Called when an error occured during decoding
		//				to update the quality information.
		// Returns:		Nothing.
		void DecodeBufferError(
			unsigned int i_From, // top line where the error occured
			unsigned int i_To	 // bottom line where the error occured
		)
		{
			COMP_TRYTHIS
			m_Image.Zero(i_From, i_To);
			m_Qinfo.Zero(i_From, i_To);
			COMP_CATCHTHIS
		}

		// Description:	Seeks the WT stream for the next restart marker. When found,
		//				update the QualityInformation and CImage accordingly.
		//				Leaves the buffer before the restart marker.
		// Returns:		true if EOI was reached,
		//				false if a valid restart marker was found.
		bool PerformResync(
			unsigned int i_Bs,
			unsigned int &t_markerNum,
			unsigned int &t_nbBlock,
			unsigned int &t_bX,
			unsigned int &t_bY);

		// Description:	Scans the WT stream for the next marker.
		//				In any case, the buffer position is set BEFORE the marker, if a
		//				marker was found.
		// Returns:		If a restart marker is found (FFE0--FFEF), its ID(0-15) is returned.
		//				Else, a negative number is returned.
		short FindNextMarker();

		// Description:	Put zeroes in specified blocks in the image.
		// Returns:		Nothing.
		void ZeroBlock(
			unsigned short i_from_line,	  // co-ordinate of the upper left corner
			unsigned short i_from_column, // "
			unsigned short i_to_line,	  // co-ordinate of the upper left corner
			unsigned short i_to_column,	  // "
			unsigned short i_bS			  // size of the blocks
		);

	public:
		// INTERFACES:

		// Description:	Constructor.
		// Returns:		Nothing.
		CWTDecoder(
			const Util::CDataFieldCompressedImage &i_Cdfci // Buffer containing the image data
														   // to decompress
			)
			: m_Image(i_Cdfci.GetNC(), i_Cdfci.GetNL(), i_Cdfci.GetNB()), m_Buf(i_Cdfci), m_Qinfo(i_Cdfci)
		{
		}

		// Description: Destructor.
		// Returns:		Nothing.
		~CWTDecoder()
		{
		}

		// Description: Performs the decoding of the input buffer.
		// Returns:		Nothing.
		void DecodeBuffer();

		// Description: Return the decompressed image. The packing is handled by CImage,
		//				that could throw a CParamException if the requested conversion
		//				is not supported.
		// Returns:		The decompressed image, with the specified representation.
		Util::CDataFieldUncompressedImage GetDecompressedImage(
			const unsigned short i_NR // Number of bits per pixel wanted
		)
		{
			COMP_TRYTHIS
			return m_Image.pack(i_NR);
			COMP_CATCHTHIS
		}

		// Description:	Returns the quality information array.
		// Returns:		A vector of one element per image line, each element containing
		//				the number of valid pixels in the corresponding line. A negative
		//				value indicates that this value is doubtful.
		std::vector<short> GetQualityInfo()
		{
			COMP_TRYTHIS
			return m_Qinfo;
			COMP_CATCHTHIS
		}
	};

} // end namespace

#endif
