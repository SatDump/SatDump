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

#include "CACModel.h"

void COMP::CACModel::Rescale()
{
	COMP_TRYTHIS_SPEED
	unsigned __int32 cum = 0UL;
	unsigned __int32 *pC = m_CumFreq + m_NbSymbols;
	unsigned __int32 *pF = m_Freq + m_NbSymbols;
	for (int i = m_NbSymbols + 1; i; i--, pF--)
	{
		*pC-- = cum;
		cum += (*pF = (*pF + 1UL) >> 1);
	}
	COMP_CATCHTHIS_SPEED
}

void COMP::CACModel::Start()
{
	COMP_TRYTHIS
	for (unsigned int i = 0; i <= m_NbSymbols; i++)
	{
		m_Freq[i] = 1;
		m_CumFreq[i] = m_NbSymbols - i;
		m_SymbolToIndex[i] = i + 1;
		m_IndexToSymbol[i] = i - 1;
	}
	m_SymbolToIndex[m_NbSymbols] = m_NbSymbols;
	m_IndexToSymbol[0] = 0;
	m_Freq[0] = 0;
	COMP_CATCHTHIS
}

void COMP::CACModel::UpdateLps(const unsigned int i_Index)
{
	COMP_TRYTHIS_SPEED
#ifdef _DEBUG
	Assert(i_Index > 1 && i_Index <= m_NbSymbols, Util::CParamException());
#endif
	if (m_CumFreq[0] >= m_MaxFreq)
		Rescale();
	unsigned int i = i_Index;
	if (m_Freq[i] == m_Freq[i - 1])
	{
		for (i--; m_Freq[i] == m_Freq[i - 1]; i--)
			;
		unsigned int symbol = m_IndexToSymbol[i];
		m_IndexToSymbol[i] = m_IndexToSymbol[i_Index];
		m_IndexToSymbol[i_Index] = symbol;
		m_SymbolToIndex[symbol] = i_Index;
		m_SymbolToIndex[m_IndexToSymbol[i]] = i;
	}
	m_Freq[i]++;
	while (i)
		m_CumFreq[--i]++;
	COMP_CATCHTHIS_SPEED
}
