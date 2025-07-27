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

#ifndef CVLCCodec_included
#define CVLCCodec_included

/*******************************************************************************

TYPE:
Concrete Class.

PURPOSE:
Common container for CVLCCoder and CVLCDecoder classes.

FUNCTION:
Provides a centralised point for declaring common member variables and functions
used by the CVLCCoder and CVLCDecoder classes.
The Reset() member function is the only interface to the class.

INTERFACES:	
See 'INTERFACES' in the module declaration below.

RESOURCES:	
Heap Memory (>2K).

REFERENCE:	
None.

PROCESSING:
The constructor just call the Reset member function.
The Reset member function initialises the models probabilities to their startup state.

DATA:
See 'DATA :' in the class header below.			

LOGIC:
None.
  
*******************************************************************************/

#include <memory>

#include "RMAErrorHandling.h"
#include "WTConst.h"
#include "CACModel.h"

namespace COMP
{

	class CVLCCodec
	{

	protected:
		// DATA :

		unsigned int m_NbBitCoef;		// Number of bits per wavelet coefficient
		unsigned int m_NbBitNbBitCoef;	// Number of bits needed to code m_NbBitCoef in binary
		unsigned int m_NbIteWt;			// Number of wavelet transform iterations
		unsigned int m_NbLossyBitPlane; // Number of wavelet transform coefficients
										// bit planes that are not coded / decoded.
										// Set according to COMP::CWTParams::m_nLossyBitPlanes
										// Value range from from 0 to 4
		unsigned int m_NbLossyQuadrant; // Number of of high frequency wavelet pyramid quadrants
										// for which the most significant bit plane must be dropped
										// Set according to COMP::CWTParams::m_nLossyBitPlanes
										// Value range from from 0 to 9
										// 0 means only the highest frequency quadrant is influenced
										// 2 means the three highest frequency quadrants are influenced
		CACModel m_Models[31][32];		// Array of multi-symbols probality models used in
										// conjuntion with the arithmetic codec.
										// First index gives the number of symbols treated by the
										// model: m_Models[i][x] treats 2+i symbols.
										// Second index is used to implement contextual probabilities
										// m_Model[i][c] is used when the previously coded
										// coefficient had a VLC magnitude value of c.
		CACModel *m_Mod;				// Pointer to avoid one indirection during coding/decoding

	public:
		// INTERFACES :

		// Description:	Reset codec initial state.
		// Returns:		Nothing.
		void Reset()
		{
			COMP_TRYTHIS
			for (unsigned int i = 0; i < 31; i++)
				for (unsigned int j = 0; j <= (i + 1); j++)
					if (m_Models[i][j].IsInitialized())
						m_Models[i][j].Initialize(0);
			COMP_CATCHTHIS
		}

		// Description:	Constructor.
		// Returns:		Nothing.
		CVLCCodec()
		{
			COMP_TRYTHIS
			Reset();
			COMP_CATCHTHIS
		}
	};

} // end namespace

#endif
