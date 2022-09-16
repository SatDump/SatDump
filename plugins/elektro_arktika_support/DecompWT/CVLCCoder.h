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

#ifndef CVLCCoder_included
#define CVLCCoder_included

/*******************************************************************************

TYPE:
Concrete class.

PURPOSE:
Encode the wavelet coefficients using a variable length coding (VLC) scheme
The entropy coding of the produced symbols is performed by an arithmetic coder
that makes use of simple contextual probabilities models.
  
FUNCTION:
The constructor takes as parameter a reference to the arithmetic coder class
that will be used to perform the entropy coding.
The Code() member function is the only interface to the class

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
The Code() member function takes as parameters a constant reference to a CWBlock class
that holds the wavelet coefficients, the number of wavelet iterations and a quantization 
step allowing to perform lossy compression by dropping one or more least significant bit 
plane(s) of some of the wavelet transform pyramid quadrants.
Starting from the DC quadrant (lowest frequency), the Code function calls the 
CodeQuadrant() private member function to code each wavelet quadrant. There are
3*ITE + 1 quadrants for ITE iterations of the wavelet transform.
Depending on the dynamic of the coefficients of the quadrant and depending on the
number of least significant bit planes to drop (lossy), the CodeQuadrant() function
initializes several symbols probability models (one for each possible context).
Then, the function codes each quadrant coefficient using the VLC method: 
the coefficient is splitted into two parts, a coefficient class part and an offset part.
The coefficient class is passed to the AC coder for entropy coding and the offset
part is passed to the AC coder for binary coding.
			
DATA:
See 'DATA :' in the class header below.

LOGIC:
				
*******************************************************************************/

#include <memory>

#include "RMAErrorHandling.h"
#include "Bitlevel.h"
#include "WTConst.h"
#include "CWBlock.h"
#include "CACCoder.h"
#include "CVLCCodec.h"

namespace COMP
{

	class CVLCCoder : public COMP::CVLCCodec
	{

	private:
		// DATA:

		CACCoder &m_Ac; // reference to the arithmetic coder

		// INTERFACES:

		// Description:	VLC encodes a wavelet coefficient.
		// Returns:		The VLC magnitude value of the coef coded.
		unsigned int CodeCoef(
			const unsigned int i_ModNum, // VLC magnitude value of the previously coded coef
			const int i_Coef			 // coef to be coded
		)
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(i_ModNum < m_Mod[0].GetNbSymbols(), Util::CParamException());
#endif
			const unsigned int m = speed_csize(i_Coef);
			m_Ac.CodeSymbol(m, m_Mod[i_ModNum]);
			if (m) {
				if (m == 1U)
					m_Ac.CodeBit(i_Coef < 0L ? 0UL : 1UL);
				else
					m_Ac.CodeBits(i_Coef < 0L ? i_Coef - 1L : i_Coef, m);
			}
			return m;
			COMP_CATCHTHIS_SPEED
		}

		// Description:	VLC encodes all DC quadrant coefficients using DPCM and a
		//				'left to right' / 'right to left' raster scan.
		// Returns:		Nothing.
		void CodeQuadrantDC(
			CWBlock &i_Wblk,			  // block containing the coefficients
			const unsigned int i_W,		  // quadrant width
			const unsigned int i_H,		  // quadrant height
			const unsigned int i_Quadrant // quadrant number (0 is highest frequency quadrant)
		);

		// Description:	VLC encodes all quadrant coefficients using a
		//				'left to right' / 'right to left' raster scan.
		// Returns:		Nothing.
		void CodeQuadrant(
			CWBlock &i_Wblk,			  // block containing the coefficients
			const unsigned int i_X,		  // top left corner x position
			const unsigned int i_Y,		  // top left corner y position
			const unsigned int i_W,		  // quadrant width
			const unsigned int i_H,		  // quadrant height
			const unsigned int i_Level,	  // unitary shift level of the quadrant
			const unsigned int i_Quadrant // quadrant number (0 is highest frequency quadrant)
		);

	public:
		// INTERFACES:

		// Description:	VLC encode the wavelet coefficients contained in the CWBlock.
		// Returns:		Nothing.
		void Code(
			CWBlock &i_Wblk,			   // block containing the coefficients
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
		CVLCCoder(
			CACCoder &i_Ac // reference to arithmetic coder class
			)
			: m_Ac(i_Ac)
		{
		}
	};

} // end namespace

#endif
