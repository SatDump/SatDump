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

#include "CACCoder.h"

void COMP::CACCoder::UpdateInterval()
{
	COMP_TRYTHIS_SPEED
	do
	{
		if (m_Low >= c_Half)
		{
			BitPlusFollow(1UL);
			m_Low -= c_Half;
		}
		else if ((m_Low + m_Range) <= c_Half)
			BitPlusFollow(0UL);
		else
		{
			m_BitsToFollow++;
			m_Low -= c_FirstQtr;
		}
		m_Low += m_Low;
		m_Range += m_Range;
	} while (m_Range <= c_FirstQtr);
	COMP_CATCHTHIS_SPEED
}

void COMP::CACCoder::CodeSymbol(const unsigned int i_Symbol, CACModel &i_Mod)
{
	COMP_TRYTHIS_SPEED
#ifdef _DEBUG
	Assert(i_Symbol < i_Mod.GetNbSymbols(), Util::CParamException());
#endif
	const unsigned int index = i_Mod.GetIndex(i_Symbol);
	const unsigned __int32 r = m_Range / i_Mod.GetCumFreq(0);
	const unsigned __int32 rLps = i_Mod.GetCumFreq(index) * r;
	m_Low += rLps;
	if (index == 1U)
	{
		m_Range -= rLps;
		i_Mod.UpdateMps();
	}
	else
	{
		m_Range = i_Mod.GetFreq(index) * r;
		i_Mod.UpdateLps(index);
	}
	if (m_Range <= c_FirstQtr)
		UpdateInterval();
	COMP_CATCHTHIS_SPEED
}
