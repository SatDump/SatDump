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

#ifndef CACModel_included
#define CACModel_included

/*******************************************************************************

TYPE:
CACModel : Concrete class.

PURPOSE:
These classes handles the adaptative symbols probabilities to be used with entropy
coding.
  
FUNCTION:
Starting from an equi-probable distribution, the model adapts itself each time a
symbol is encoded by managing symbols frequency counts, cumulative frequency and
frequency-based symbol sorting.
	
INTERFACES:	
See 'INTERFACES' in the module declaration below
      
RESOURCES:	
Heap Memory (>2K).
		
REFERENCE:	
"Arithmetic Coding for Data Compression"; Witten, Neal & Cleary;
Commun. ACM, vol. 30, pp 520-540, June 1987.
		  
PROCESSING:	
The symbols probabilities are initially equal, frequency counts are set to 1, 
cumulative frequencies are computed according to initial symbol ordering.
Each time a symbol is encoded a model update is performed, the symbol frequency count 
is incremented, a move-to-front technique is used to keep the symbols sorted
by frequency and the cumulative frequencies are computed appropriatelly.
When the the sum of the frequency counts exceeds a given threshold, the symbols 
frequencies are scaled down by half to give more weight to more recents events.
			
DATA:
See 'DATA :' in the class header below.			

LOGIC:

*******************************************************************************/

#include <memory>

#include "RMAErrorHandling.h"
#include "Bitlevel.h"
#include "WTConst.h"

namespace COMP
{
	class CACModel
	{
	private:
		// DATA:

		const unsigned __int32 c_MaxFrequency; // upper limit for cumulative threshold
		unsigned __int32 m_MaxFreq;			   // cumulative frequency threshold
		unsigned int m_NbSymbols;			   // number of symbol handled by the model
		unsigned __int32 m_Freq[33];		   // frequency counts array
		unsigned __int32 m_CumFreq[33];		   // cumulative frequency counts array
		unsigned int m_SymbolToIndex[33];	   // symbol to sorting index array
		unsigned int m_IndexToSymbol[33];	   // sorting index to symbol array

		// INTERFACES:

		// Description: Performs the symbol frequencies rescaling.
		// Returns:		Nothing.
		void Rescale();

	public:
		// INTERFACES:

		// Description:	Default constructor.
		// Returns:		Nothing.
		CACModel()
			: c_MaxFrequency((1UL << (c_ACNbBits - 2)) - 1), m_NbSymbols(0)
		{
		}

		// Description:	Get the number of symbols handled by the model [0, 32].
		// Returns:		The number of symbols.
		unsigned int GetNbSymbols() const
		{
			return m_NbSymbols;
		}

		// Description: Get the frequency of the symbol given by its sorting index.
		//				1 is the most probable symbol, GetNbSymbols() is the least
		//				probable one.
		// Returns:		The symbol frequency count.
		unsigned __int32 GetFreq(
			const unsigned int i_Index // sorting index of the symbol
		)
			const
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(i_Index > 0 && i_Index <= m_NbSymbols, Util::CParamException());
#endif
			return m_Freq[i_Index];
			COMP_CATCHTHIS_SPEED
		}

		// Description: Get the cumulative frequency at a given sorting index.
		//				1 is the most probable symbol, GetNbSymbols() is the least
		//				probable one,
		//				GetCumFreq(i) is equal to the sum of GetFreq(k) with
		//				k=[i+1, GetNbSymbols()]
		// Returns:		The cumulative frequency.
		unsigned __int32 GetCumFreq(
			const unsigned int i_Index // sorting index
		)
			const
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(i_Index <= m_NbSymbols, Util::CParamException());
#endif
			return m_CumFreq[i_Index];
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Get the sorting index of a given symbol.
		// Returns:		The sorting index.
		unsigned int GetIndex(
			const unsigned int i_Symbol // the symbol [0, GetNbSymbols()[
		)
			const
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(i_Symbol < m_NbSymbols, Util::CParamException());
#endif
			return m_SymbolToIndex[i_Symbol];
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Get the symbol associated with a given sorting index.
		// Returns:		The associated symbol.
		unsigned int GetSymbol(
			const unsigned int i_Index // the sorting index [1, GetNbSymbols()]
		)
			const
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(i_Index > 0 && i_Index <= m_NbSymbols, Util::CParamException());
#endif
			return m_IndexToSymbol[i_Index];
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Check if the model has been initialised.
		// Returns:		true if the model was initialized, false otherwise.
		bool IsInitialized() const
		{
			COMP_TRYTHIS_SPEED
			return m_NbSymbols ? true : false;
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Reset the symbol frequencies to the initial state (equi-probable).
		// Returns:		Nothing.
		void Start();

		// Description:	Update the model after the encoding of the MPS.
		// Returns:		Nothing.
		void UpdateMps()
		{
			COMP_TRYTHIS_SPEED
			if (m_CumFreq[0] >= m_MaxFreq)
				Rescale();
			m_Freq[1]++;
			m_CumFreq[0]++;
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Update the model after the encoding of
		//				one of the LPS given by its sorting index.
		// Returns:		Nothing.
		void UpdateLps(
			const unsigned int i_Index // the sorting index
		);

		// Description:	Initialise the model to handle a given number of symbols.
		// Returns:		Nothing.
		void Initialize(
			unsigned int i_NbS // the number of symbols [0, 32]
		)
		{
			COMP_TRYTHIS
			Assert(i_NbS <= 32, Util::CParamException());
			m_NbSymbols = i_NbS;
			if (i_NbS)
			{
				unsigned __int32 temp = (unsigned __int32)m_NbSymbols << 5;
				m_MaxFreq = (temp < c_MaxFrequency) ? temp : c_MaxFrequency;
				Start();
			}
			COMP_CATCHTHIS
		}
	};

} // end namespace

#endif
