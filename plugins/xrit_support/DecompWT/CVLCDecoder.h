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

#ifndef CVLCDecoder_included
#define CVLCDecoder_included

/*******************************************************************************

TYPE:
Concrete class.

PURPOSE:
Decode the wavelet coefficients using a variable length coding (VLC) scheme
The entropy decoding of the symbols is performed by an arithmetic decoder that
makes use of simple contextual probabilities models.
  
FUNCTION:
The constructor takes as parameter a reference to the arithmetic decoder class
that will be used to perform the entropy decoding.
The Decode() member function is the only interface to the class
	
INTERFACES:	
See 'INTERFACES' in the module declaration below
      
RESOURCES:	
Heap Memory (>2K).
		
REFERENCE:	
Digital compression and coding of continuous-tone still images.
Part 1, Requirements and guidelines, ISO/IEC 10918-1.
"An Image Multiresolution Representation for Lossless and Lossy Compression",
Amir Said, William A. Pearlman. SPIE Symposium on Visual Communications and
Image Processing, Cambridge, MA, Nov. 1993.
		  
PROCESSING:
The Decode() member function takes as parameters a reference to a CWBlock class
where to store the decoded wavelet coefficients, the number of wavelet iterations 
and a quantization step allowing to perform lossy decompression by dropping one or 
more least significant bit plane(s) of some of the wavelet transform pyramid quadrants.
Starting from the DC quadrant (lowest frequency), the Decode function calls the 
DecodeQuadrant() private member function to decode each wavelet quadrant. There are
3*ITE + 1 quadrants for ITE iterations of the wavelet transform.
Depending on the dynamic of the coefficients of the quadrant and depending on the
number of least significant bit planes to drop (lossy), the DecodeQuadrant() function
initializes several symbols probability models (one for each possible context).
Then, the function decodes each quadrant coefficient using the VLC method: 
the coefficient is splitted into two parts, a coefficient class part and an offset part.
The coefficient class is entropy decoded by the AC decoder and the offset
part is binary decoded by the AC decoder.
			
DATA:
See 'DATA :' in the class header below.			

LOGIC:
				
*******************************************************************************/

#include <memory>

#include "RMAErrorHandling.h"

#include "Bitlevel.h"

#include "WTConst.h"
#include "CWBlock.h"
#include "CACDecoder.h"
#include "CVLCCodec.h"

namespace COMP
{

	class CVLCDecoder : public COMP::CVLCCodec
	{

	private:
		// DATA:

		CACDecoder &m_Ad; // reference to the arithmetic decoder

		// INTERFACES:

		// Description:	VLC decodes a wavelet coefficient.
		// Returns:		The VLC magnitude value of the coef decoded.
		unsigned int DecodeCoef(
			const unsigned int i_ModNum, // VLC magnitude value of
										 // the previously decoded coef.
			int &o_Coef					 // reference to coef to be decoded.
		)
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(i_ModNum < m_Mod[0].GetNbSymbols(), Util::CParamException());
#endif
			const unsigned int m = m_Ad.DecodeSymbol(m_Mod[i_ModNum]);
			if (m)
				if (m == 1U)
				{
					o_Coef = m_Ad.DecodeBit() ? 1L : -1L;
				}
				else
				{
					o_Coef = m_Ad.DecodeBits(m);
					const int mask = 1L << (m - 1U);
					if (!(o_Coef & mask))
						o_Coef -= mask + mask - 1;
				}
			else
				o_Coef = 0L;
			return m;
			COMP_CATCHTHIS_SPEED
		}

		// Description:	VLC decodes all DC quadrant coefficients using a DPCM and a
		//				'left to right' / 'right to left' raster scan.
		// Returns:		true if decoding is successful, false if an error occurred.
		bool DecodeQuadrantDC(
			CWBlock &o_Wblk,			  // reference to block containing the coefficients
			const unsigned int i_W,		  // quadrant width
			const unsigned int i_H,		  // quadrant height
			const unsigned int i_Quadrant // quadrant number (0 is highest frequency quadrant)
		);

		// Description:	VLC decodes all quadrant coefficients using a
		//				'left to right' / 'right to left' raster scan.
		// Returns:		true if decoding is successful, false if an error occurred.
		bool DecodeQuadrant(
			CWBlock &o_Wblk,			  // reference to block containing the coefficients
			const unsigned int i_X,		  // top left corner x position
			const unsigned int i_Y,		  // top left corner y position
			const unsigned int i_W,		  // quadrant width
			const unsigned int i_H,		  // quadrant height
			const unsigned int i_Level,	  // unitary shift level of the quadrant
			const unsigned int i_Quadrant // quadrant number (0 is highest frequency quadrant)
		);

		// Description:	Refine the decoded coefs of a quadrant by halving the error interval.
		// Returns:		Nothing.
		void RefineLossyQuadrant(
			CWBlock &o_Wblk,			  // reference to block containing the coefficients
			const unsigned int i_X,		  // top left corner x position
			const unsigned int i_Y,		  // top left corner y position
			const unsigned int i_W,		  // quadrant width
			const unsigned int i_H,		  // quadrant height
			const unsigned int i_Level,	  // unitary shift level of the quadrant
			const unsigned int i_Quadrant // quadrant number (0 is highest frequency quadrant)
		);

		// Description:	Refine the decoded coefs of a block by halving the error interval.
		// Returns:		Nothing.
		void RefineLossy(
			CWBlock &o_Wblk // reference to block containing the coefficients
		);

	public:
		// INTERFACES :

		// Description:
		// Returns:		true if decoding is successful, false if an error occurred.
		bool Decode(
			CWBlock &o_Wblk,			   // reference to the block where to store the coefficients
			const unsigned int i_NbIteWt,  // number of wavelet iterations
			const unsigned int i_NbLossyBp // Number of wavelet transform coefficients
										   // bit planes that are not coded / decoded
										   // Lossless if m_nLossyBitPlanes = 0
										   // Lossy if m_nLossyBitPlanes > 0 [1-15]
										   // 1 ->  1 lossy bit plane
										   // 4 ->  2 lossy bit planes
										   // 9 ->  3 lossy bit planes
										   // 15 -> 4 lossy bit planes
										   // others -> intermediate levels
		);

		// Description:	Constructor.
		// Returns:		Nothing.
		CVLCDecoder(
			CACDecoder &i_Ad // reference to arithmetic decoder class
			)
			: m_Ad(i_Ad)
		{
		}
	};

} // end namespace

#endif
