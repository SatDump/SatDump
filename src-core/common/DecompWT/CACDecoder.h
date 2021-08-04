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

#ifndef CACDecoder_included
#define CACDecoder_included

/*******************************************************************************

TYPE:
Concrete Class.

PURPOSE:
This class handles the entropy decoding of symbols by means of arithmetic decoding.
  
FUNCTION:
Given a multi-symbols probality model this class allows to decode a sequence of
symbols by recovering it from a real number reflecting the probability of the sequence.
It also allows decoding a binary sequence using an equi-probable symbols model.
The decoding of multi-symbols and equi-probable symbols can be freely mixed using
one common input stream.
	
INTERFACES:	
See 'INTERFACES' in the module declaration below
      
RESOURCES:	
Heap Memory (>2K).
		
REFERENCE:	
"Arithmetic Coding for Data Compression"; Witten, Neal & Cleary;
Commun. ACM, vol. 30, pp 520-540, June 1987.
		  
PROCESSING:	
The initial decoding range is set to [0, 2^c_ACNbBits[
A value window (c_ACNbBits bits) is initialized so that it contains the c_ACNbBits
most significant bits of the real number binary representation.
Given the current value window and a symbols probability model, the arithmetic decoder
can find the symbol that has been encoded.
Each time a symbol is decoded this range is reduced according to the
symbol probability. If the range falls below a given size, the current value window 
is moved right by one or more bits and the range is rescaled.
Each time a bit is read from the input buffer, we look for the presence of a marker;
a flag is activated if a marker is present and the read is canceled.
	
DATA:
See 'DATA :' in the class header below.	

LOGIC:

*******************************************************************************/

#include <memory>

#include "RMAErrorHandling.h"
#include "CBuffer.h"
#include "WTConst.h"
#include "CACModel.h"

namespace COMP
{
	class CACDecoder
	{
	private:
		// DATA :

		const unsigned __int32 c_TopValue; // range maximum size - 1
		const unsigned __int32 c_FirstQtr; // a quarter of the range maximum size
		unsigned __int32 m_Value;		   // current value window
		unsigned __int32 m_Range;		   // current range
		bool m_MarkerReached;			   // flag indicating if a marker has been eaten
		CRBuffer &m_Buf;				   // the input buffer

		// PRIVATE FUNCTIONS :

		// Description:	Read a binary string form the input stream.
		// Returns:		The binary string.
		unsigned __int32 InputBits(
			const unsigned int i_NbBits // the number of bits in the binary string
		)
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(i_NbBits > 0 && i_NbBits <= 32, Util::CParamException());
#endif
			if (m_Buf.in_marker(i_NbBits))
			{
				m_MarkerReached = true;
#ifdef _DEBUG
				std::cerr << "Reading a marker !" << std::endl;
#endif
				return 0;
			}
			const unsigned __int32 bits = m_Buf.read32() >> (32U - i_NbBits);
			m_Buf.seek(i_NbBits);
			return bits;
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Eventually rescale the decoding range and move the value window.
		// Returns:		Nothing.
		void UpdateInterval();

	public:
		// INTERFACES :

		// Description:	Constructor.
		// Returns:		Nothing.
		CACDecoder(
			CRBuffer &i_Buf // the input buffer
			)
			: c_TopValue((1UL << c_ACNbBits) - 1), c_FirstQtr(1UL << (c_ACNbBits - 2)), m_Buf(i_Buf)
		{
		}

		// Description:	Tell if a marker was found during previous read of the input buffer.
		// Returns:		true if a marker was found, false otherwise.
		bool IsMarkerReached() const
		{
			return m_MarkerReached;
		}

		// Description:	Start and initialise the arithmetic decoding.
		// Returns:		Nothing.
		void Start()
		{
			COMP_TRYTHIS
			m_MarkerReached = false;
			m_Range = c_TopValue + 1UL;
			m_Value = InputBits(c_ACNbBits);
			COMP_CATCHTHIS
		}

		// Description:	Decode a symbol using a multi-symbols probability model.
		// Returns:		The symbol decoded.
		unsigned int DecodeSymbol(
			CACModel &i_Mod // the multi-symbols probability model
		);

		// Description:	Decode a binary sequence using an equi-probable symbols model.
		// Returns:		The binary sequence.
		unsigned __int32 DecodeBits(
			const unsigned int i_NbBits // the number of bits of the sequence
		)
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(i_NbBits > 0 && i_NbBits <= (c_ACNbBits - 2), Util::CParamException());
#endif
			m_Range >>= i_NbBits;
			const unsigned __int32 bits = m_Value / m_Range;
			m_Value -= bits * m_Range;
			if (m_Range <= c_FirstQtr)
				UpdateInterval();
			return bits;
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Decode a bit using an equi-probable symbols model.
		// Returns:		The bit decoded.
		unsigned __int32 DecodeBit()
		{
			COMP_TRYTHIS_SPEED
			m_Range >>= 1;
			const unsigned __int32 bit = m_Value >= m_Range ? 1UL : 0UL;
			if (bit)
				m_Value -= m_Range;
			if (m_Range <= c_FirstQtr)
				UpdateInterval();
			return bit;
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Stop the arithmetic decoding.
		// Returns:		Nothing.
		void Stop()
		{
		}
	};

} // end namespace

#endif
