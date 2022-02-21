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

#ifndef CACCoder_included
#define CACCoder_included

/*******************************************************************************

TYPE:
Concrete Class.

PURPOSE:
This class handles the entropy coding of symbols by means of arithmetic coding.
  
FUNCTION:
Given a multi-symbols probality model this class allows to entropy code a sequence
of symbols by outputing a real number reflecting the probability of the sequence.
It also allows encoding a binary sequence using an equi-probable symbols model.
The encoding of multi-symbols and equi-probable symbols can be freely mixed in one
common output stream.
	
INTERFACES:	
See 'INTERFACES' in the module declaration below
      
RESOURCES:	
Heap Memory (>2K).
		
REFERENCE:
"Arithmetic Coding for Data Compression"; Witten, Neal & Cleary;
Commun. ACM, vol. 30, pp 520-540, June 1987.
		  
PROCESSING:
The initial coding range is set to [0, 2^c_ACNbBits[
Each time a symbol is encoded this range is reduced according to the
symbol probability. If the range falls below a given size, one or more bits
are output (most significant bits of the real number expressed in binary) and
the range is rescaled.
When the coder is stopped the trailing bits (c_ACNbBits bits) are output to 
disambiguate the sequence of symbols from any other one.
			
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
	class CACCoder
	{
	private:
		// DATA :

		const unsigned __int32 c_TopValue; // range maximum size - 1
		const unsigned __int32 c_FirstQtr; // a quarter of the range maximum size
		const unsigned __int32 c_Half;	   // a half of the range maximum size
		unsigned __int32 m_Low;			   // base value of the current range
		unsigned __int32 m_Range;		   // current range size
		unsigned __int32 m_BitsToFollow;   // number of opposite bits that must follow
										   // the next bit output
		unsigned int m_NbBits;			   // available bits in output accumulator [0,32]
		unsigned __int32 m_Bits;		   // output bits accumulator
		CWBuffer &m_Buf;				   // output stream buffer

		// PRIVATE FUNCTIONS :

		// Description:	Output one bit to the CWBuffer.
		// Returns:		Nothing.
		void OutputBit(
			const unsigned __int32 i_Bit // the bit state, 0 or 1
		)
		{
			COMP_TRYTHIS_SPEED
			m_Bits += m_Bits + i_Bit;
			if (!(--m_NbBits))
			{
				m_Buf.write32_aligned(m_Bits);
				m_NbBits = 32U;
				m_Bits = 0UL;
			}
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Flush the remaining bits of the output accumulator.
		// Returns:		Nothing.
		void FlushBits()
		{
			COMP_TRYTHIS
			if (m_NbBits < 32U)
				m_Buf.write(m_Bits, 32U - m_NbBits);
			COMP_CATCHTHIS
		}

		// Description:	Output one bit followed by several opposite bits.
		// Returns:		Nothing.
		void BitPlusFollow(
			const unsigned __int32 i_Bit // the bit state, 0 or 1
		)
		{
			COMP_TRYTHIS_SPEED
			OutputBit(i_Bit);
			while (m_BitsToFollow)
			{
				OutputBit(1UL - i_Bit);
				m_BitsToFollow--;
			}
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Eventually rescale the coding range and output some bits.
		// Returns:		Nothing.
		void UpdateInterval();

	public:
		// INTERFACES :

		// Description:	Constructor.
		// Returns:		Nothing.
		CACCoder(
			CWBuffer &i_Buf // the output buffer
			)
			: c_TopValue((1UL << c_ACNbBits) - 1), c_FirstQtr(1UL << (c_ACNbBits - 2)), c_Half(c_FirstQtr << 1), m_Buf(i_Buf)
		{
		}

		// Description:	Start and initialise the arithmetic coder.
		// Returns:		Nothing.
		void Start()
		{
			COMP_TRYTHIS
			m_Low = 0UL;
			m_Range = c_TopValue + 1UL;
			m_BitsToFollow = 0UL;
			m_NbBits = 32U;
			m_Bits = 0UL;
			COMP_CATCHTHIS
		}

		// Description:	Code the given symbol using its probability given
		//				by the multi-symbols model.
		// Returns:		Nothing.
		void CodeSymbol(
			const unsigned int i_Symbol, // the symbol to encode, [0, N[
			CACModel &i_Mod				 // the multi-symbols probability model
		);

		// Description:	Encode a binary sequence using an equi-probable symbols model.
		// Returns:		Nothing.
		void CodeBits(
			const unsigned __int32 i_Bits, // the binary sequence
			const unsigned int i_NbBits	   // the number of bits of the sequence
		)
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(i_NbBits > 0 && i_NbBits <= (c_ACNbBits - 2), Util::CParamException());
#endif
			m_Range >>= i_NbBits;
			m_Low += (i_Bits & ((1UL << i_NbBits) - 1UL)) * m_Range;
			if (m_Range <= c_FirstQtr)
				UpdateInterval();
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Encode a bit using an equi-probable symbols model.
		// Returns:		Nothing.
		void CodeBit(
			const unsigned __int32 i_Bit // the bit to code
		)
		{
			COMP_TRYTHIS_SPEED
			m_Range >>= 1;
			if (i_Bit)
				m_Low += m_Range;
			if (m_Range <= c_FirstQtr)
				UpdateInterval();
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Stop the arithmetic coding and flush the remaining bits.
		// Returns:		Nothing.
		void Stop()
		{
			COMP_TRYTHIS
			for (unsigned int i = c_ACNbBits; i; i--)
				BitPlusFollow(m_Low & speed_bit32(i) ? 1UL : 0UL);
			FlushBits();
			COMP_CATCHTHIS
		}
	};

} // end namespace

#endif
