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

#ifndef CWTCoder_included
#define CWTCoder_included

/*******************************************************************************

TYPE:
Concrete class.

PURPOSE: 
Base Wavelet Coder object featuring common functionalities between all possible coders.

FUNCTION:	
This object is merely a container for common Wavelet-coder sub-objects
and is used to have a centralised initialisation point.

INTERFACES:	
See 'INTERFACES' in the module declaration below 

RESOURCES:	
Heap Memory (>2K).

REFERENCE:	
None.

PROCESSING:	
The constructor takes a CDataFieldUncompressed image and a CCompressParam
and uses these to initilise the subobjects.
The CodeBuffer member function starts the actual coding.
The GetCompressedImage returns the CDataFieldCompressedImage after coding

DATA:
See 'DATA' in the module declaration below 

LOGIC:

*******************************************************************************/

#include "CDataField.h"
#include "CompressWT.h"
#include "CImage.h"
#include "CBuffer.h"

namespace COMP
{

	class CWTCoder
	{

	private:
		// DATA:

		CImage m_Image;	   //	image data buffer
		CWTParams m_Param; //	compression parameters
		CWBuffer m_Buf;	   //	compressed data buffer
		
		enum E_constants
		{
			e_ExpectedCompressionFactor = 1, // Expected compression factor, used
											 // to allocate the first CWBuffer.
		};

		// INTERFACES:

		// Description:	Performs the coding of the input buffer by block.
		// Returns:		Nothing.
		void CodeBufferBlock(
			const unsigned int i_Bs // block size
		);

		// Description: Performs the coding of the input buffer as a whole.
		// Returns:		Nothing.
		void CodeBufferFull();

	public:
		// INTERFACES:

		// Description:	Creates a CWTCoder object, constructor.
		// Returns:		Nothing.
		CWTCoder(
			const Util::CDataFieldUncompressedImage &i_Cdfui, // buffer containing the image
															  // data to compress
			const CWTParams &i_Param						  // parameters of the compression
			)
			: m_Image(i_Cdfui), m_Param(i_Param), m_Buf((unsigned __int32)(i_Cdfui.GetLength() * e_ExpectedCompressionFactor / 8))
		{
			PRECONDITION(m_Image.GetW() >= 1 && m_Image.GetH() >= 1);
			PRECONDITION(m_Param.m_BitsPerPixel >= 1 && m_Param.m_BitsPerPixel <= 16);
			PRECONDITION(m_Param.m_nWTlevels >= 3 && m_Param.m_nWTlevels <= 6);
			PRECONDITION(m_Param.m_nLossyBitPlanes <= 15);
		}

		// Description:	Destructor.
		// Returns:		Nothing.
		~CWTCoder()
		{
		}

		// Description:	Performs the coding of the input buffer in the output buffer.
		// Returns:		Nothing.
		void CodeBuffer();

		// Description:	Returns a CDFCI containing the compressed data buffer.
		// Returns:		The compressed data buffer.
		Util::CDataFieldCompressedImage GetCompressedImage()
		{
			COMP_TRYTHIS
			Util::CDataFieldCompressedImage cdfci(m_Buf,
												  (unsigned char)m_Image.GetNB(),
												  m_Image.GetW(),
												  m_Image.GetH());
			return cdfci;
			COMP_CATCHTHIS
		}
	};

} // end namespace

#endif
