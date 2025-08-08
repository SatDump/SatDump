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

//:Ignore
//////////////////////////////////////////////////////////////////////////
//
//	FileName:		CImage.cpp
//
//	Date Created:	14/8/1998
//
//	Author:			Van Wynsberghe Laurent
//
//
//	Description:	pack and unpack functions, and random packed buffer generator
//
//
//	Last Modified:	$Dmae: 1999/04/20 08:49:05 $
//
//  RCS Id:			$Id: CImage.cpp,v 1.34 1999/04/20 08:49:05 youag Exp $
//
//	$Rma: CImage.cpp,v $
//	Revision 1.34  1999/04/20 08:49:05  youag
//	Optimizing CImage pack & unpack for NR = 8, 10, 12 and 16
//
//	Revision 1.33  1999/04/16 17:03:31  youag
//	Optimizing pack and unpack functions using *pointer++
//
//	Revision 1.32  1999/04/16 11:03:32  youag
//	Cleaning the code from commented lines
//
//	Revision 1.31  1999/04/15 17:49:57  youag
//	Buf correction in CImage pack(), bug introduced by previous optimization
//
//	Revision 1.30  1999/04/15 15:24:20  youag
//	CImage optimization of pack and unpack functions (removing un-needed masks)
//
//	Revision 1.29  1999/03/24 12:53:40  youag
//	Intel / Alpha optimisation tests
//
//	Revision 1.28  1999/03/23 15:40:26  xne
//	Optimized Cget/Cset data members for alpha
//
//	Revision 1.27  1999/03/01 20:07:40  xne
//	(Re)introducing CVS/RCS commit messages
//
////////////////////////////////////////////////////////////////////////////
//:End Ignore

#include <memory>
#include <iostream>

#include "CImage.h"

namespace COMP
{
	//------------------------------------------------------------------------
	CImage::CImage(unsigned short i_NC, unsigned short i_NL, unsigned short i_NB)
	{
		COMP_TRYTHIS
		Resize(i_NC, i_NL, i_NB);
		COMP_CATCHTHIS
	}
	//------------------------------------------------------------------------
	CImage::CImage(const Util::CDataFieldUncompressedImage &i_cdfui)
	{
		COMP_TRYTHIS
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// Optimized version for (NB == NR) in 8, 10 or 12 bits
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		Assert(i_cdfui.GetNR() >= i_cdfui.GetNB(), Util::CParamException());
		Assert((i_cdfui.GetNR() == 8) ||
				   (i_cdfui.GetNR() == 10) ||
				   (i_cdfui.GetNR() == 12) ||
				   (i_cdfui.GetNR() == 16),
			   Util::CParamException());

		Resize(i_cdfui.GetNC(), i_cdfui.GetNL(), i_cdfui.GetNB());

		unsigned short word_IN, word_OUT;

		const unsigned char *pIn = i_cdfui.get();
		unsigned short *pOut = &m_data[0];

		ResetState();
		switch (i_cdfui.GetNR())
		{
		case 16:
			while (m_index < m_size)
			{
				word_IN = *pIn++;
				word_OUT = word_IN << 8; // 1111.1111
				word_IN = *pIn++;
				word_OUT |= word_IN; // 1111.1111
				*pOut++ = word_OUT;
				m_index++;
			}
			break;
		case 12:
			while (m_index < m_size)
			{
				// read 2 pixels = 3 bytes and write 2 pixels = 4 bytes
				word_IN = *pIn++;
				word_OUT = word_IN << 4; // 1111.1111
				word_IN = *pIn++;
				word_OUT |= word_IN >> 4; // 1111.0000
				*pOut++ = word_OUT;
				m_index++;
				if (m_index < m_size)
				{
					word_OUT = (word_IN & 0x0F) << 8; // 0000.1111
					word_IN = *pIn++;
					word_OUT |= word_IN; // 1111.1111
					*pOut++ = word_OUT;
					m_index++;
				}
			}
			break;
		case 10:
			while (m_index < m_size)
			{
				// read 5 bytes=4 pixels and write 4 pixels=8 bytes
				word_IN = *pIn++;
				word_OUT = word_IN << 2; // 1111.1111
				word_IN = *pIn++;
				word_OUT |= word_IN >> 6; // 1100.0000
				*pOut++ = word_OUT;
				m_index++;
				if (m_index < m_size)
				{
					word_OUT = (word_IN & 0x3F) << 4; // 0011.1111
					word_IN = *pIn++;
					word_OUT |= word_IN >> 4; // 1111.0000
					*pOut++ = word_OUT;
					m_index++;
					if (m_index < m_size)
					{
						word_OUT = (word_IN & 0x0F) << 6; // 0000.1111
						word_IN = *pIn++;
						word_OUT |= word_IN >> 2; // 1111.1100
						*pOut++ = word_OUT;
						m_index++;
						if (m_index < m_size)
						{
							word_OUT = (word_IN & 0x03) << 8; // 0000.0011
							word_IN = *pIn++;
							word_OUT |= word_IN; // 1111.1111
							*pOut++ = word_OUT;
							m_index++;
						}
					}
				}
			}
			break;
		case 8:
			while (m_index < m_size)
			{
				word_IN = *pIn++;
				word_OUT = word_IN; // 1111.1111
				*pOut++ = word_OUT;
				m_index++;
			}
			break;
		default:
			// Not (yet) implemented
			Assert(0, Util::CParamException());
		}
		ResetState();
		COMP_CATCHTHIS
	}
	//------------------------------------------------------------------------
	void CImage::Resize(unsigned short i_NC, unsigned short i_NL, unsigned short i_NB)
	{
		COMP_TRYTHIS
		m_NB = i_NB;
		m_NC = i_NC;
		m_NL = i_NL;
		m_size = (unsigned long)i_NC * i_NL;

		m_data.clear();
		m_dataptr.clear();
		if (m_size > 0)
		{
			m_data.resize(m_size, 0);
			Assert(m_data.size() == m_size, Util::CCLibException());
			m_dataptr.resize(m_NL, NULL);
			Assert(m_dataptr.size() == m_NL, Util::CCLibException());
			for (short i = 0; i < m_NL; i++)
				m_dataptr[i] = &m_data[i * m_NC];
		}
		ResetState();
		COMP_CATCHTHIS
	}
	//------------------------------------------------------------------------
	void CImage::Forward_point_transform(unsigned short i_pt)
	{
		COMP_TRYTHIS
		// Perform the point transform on every pixel
		// pixel = pixel / 2^(i_pt)
		Assert(i_pt <= 16, Util::CParamException());
		if (i_pt)
			for (unsigned long i = 0; i < m_size; i++)
				m_data[i] >>= i_pt;
		COMP_CATCHTHIS
	}
	//------------------------------------------------------------------------
	void CImage::Inverse_point_transform(unsigned short i_pt)
	{
		COMP_TRYTHIS
		// Perform the inverse point transform on every pixel
		// pixel = pixel * 2^(i_pt)
		Assert(i_pt <= 16, Util::CParamException());
		if (i_pt)
			for (unsigned long i = 0; i < m_size; i++)
				m_data[i] <<= i_pt;
		COMP_CATCHTHIS
	}
	//------------------------------------------------------------------------
	Util::CDataFieldUncompressedImage CImage::pack(unsigned short i_NR)
	{
		COMP_TRYTHIS
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// Optimized version for (NB == NR) in 8, 10 or 12 bits
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		Assert(i_NR >= m_NB, Util::CParamException()); // this check is also performed in the CDFUI constructor
		Assert(i_NR == 8 ||
				   i_NR == 10 ||
				   i_NR == 12 ||
				   i_NR == 16,
			   Util::CParamException());

		Util::CDataFieldUncompressedImage cdfui((unsigned char)m_NB, m_NC, m_NL, (unsigned char)i_NR);

		unsigned short word_IN, word_OUT;
		const unsigned short *pIn = &m_data[0];
		unsigned char *pOut = cdfui.get();

		ResetState();
		switch (i_NR)
		{
		case 16:
			while (m_index < m_size)
			{
				// read 1 pixel = 2 bytes and write 1 pixel = 2 bytes
				word_IN = *pIn++;
				m_index++;
				word_OUT = word_IN >> 8; // 1111.1111.0000.0000
				*pOut++ = (unsigned char)word_OUT;
				word_OUT = word_IN & 0xFF; // 0000.0000.1111.1111
				*pOut++ = (unsigned char)word_OUT;
			}
			break;
		case 12:
			while (m_index < m_size)
			{
				// read 2 pixels = 4 bytes and write 2 pixels = 3 bytes
				word_IN = *pIn++ & 0xFFF;
				m_index++;
				word_OUT = word_IN >> 4; // 1111.1111.0000
				*pOut++ = (unsigned char)word_OUT;
				word_OUT = (word_IN & 0x00F) << 4; // 0000.0000.1111
				if (m_index < m_size)
				{
					word_IN = *pIn++ & 0xFFF;
					m_index++;
					word_OUT |= word_IN >> 8; // 1111.0000.0000
					*pOut++ = (unsigned char)word_OUT;
					word_OUT = word_IN & 0x0FF; // 0000.1111.1111
					*pOut++ = (unsigned char)word_OUT;
				}
				else
					*pOut++ = (unsigned char)word_OUT;
			}
			break;
		case 10:
			while (m_index < m_size)
			{
				// read 4 pixels = 8 bytes and write 4 pixels = 5 bytes
				word_IN = *pIn++ & 0x3FF;
				m_index++;
				word_OUT = word_IN >> 2; // 11.1111.1100
				*pOut++ = (unsigned char)word_OUT;
				word_OUT = (word_IN & 0x003) << 6; // 00.0000.0011
				if (m_index < m_size)
				{
					word_IN = *pIn++ & 0x3FF;
					m_index++;
					word_OUT |= word_IN >> 4; // 11.1111.0000
					*pOut++ = (unsigned char)word_OUT;
					word_OUT = (word_IN & 0x00F) << 4; // 00.0000.1111
					if (m_index < m_size)
					{
						word_IN = *pIn++ & 0x3FF;
						m_index++;
						word_OUT |= word_IN >> 6; // 11.1100.0000
						*pOut++ = (unsigned char)word_OUT;
						word_OUT = (word_IN & 0x03F) << 2; // 00.0011.1111
						if (m_index < m_size)
						{
							word_IN = *pIn++ & 0x3FF;
							m_index++;
							word_OUT |= word_IN >> 8; // 11.0000.0000
							*pOut++ = (unsigned char)word_OUT;
							word_OUT = word_IN & 0x0FF; // 00.1111.1111
							*pOut++ = (unsigned char)word_OUT;
						}
						else
							*pOut++ = (unsigned char)word_OUT;
					}
					else
						*pOut++ = (unsigned char)word_OUT;
				}
				else
					*pOut++ = (unsigned char)word_OUT;
			}
			break;
		case 8:
			while (m_index < m_size)
			{
				word_IN = *pIn++ & 0xFF;
				m_index++;
				word_OUT = word_IN; // 1111.1111
				*pOut++ = (unsigned char)word_OUT;
			}
			break;
		default:
			// Not (yet) implemented
			Assert(0, Util::CParamException());
		}
		ResetState();
		return cdfui;
		COMP_CATCHTHIS
	}
	//------------------------------------------------------------------------
	void CImage::Requantize(COMP::ERequantizationMode i_mode)
	{
		COMP_TRYTHIS
		unsigned long i;

		unsigned short *pData = &m_data[0];
		switch (i_mode)
		{
		case COMP::e_NoRequantization:
			return;
		case COMP::e_10to12:
			Assert(m_NB == 10, Util::CParamException());
			m_NB = 12;
			return;
		case COMP::e_10to8_floor:
			Assert(m_NB == 10, Util::CParamException());
			for (i = 0; i < m_size; i++)
				*pData++ >>= 2;
			m_NB = 8;
			return;
		case COMP::e_10to8_floor1:
			Assert(m_NB == 10, Util::CParamException());
			for (i = 0; i < m_size; i++)
			{
				unsigned short c = *pData;
				c = (c + 1) >> 2;
				if (c > 255)
					c = 255;
				*pData++ = c;
			}
			m_NB = 8;
			return;
		case COMP::e_10to8_round:
			Assert(m_NB == 10, Util::CParamException());
			for (i = 0; i < m_size; i++)
			{
				unsigned short c = *pData;
				c = (c + 2) >> 2;
				if (c > 255)
					c = 255;
				*pData++ = c;
			}
			m_NB = 8;
			return;
		case COMP::e_10to8_ceil:
			Assert(m_NB == 10, Util::CParamException());
			for (i = 0; i < m_size; i++)
			{
				unsigned short c = *pData;
				c = (c + 3) >> 2;
				if (c > 255)
					c = 255;
				*pData++ = c;
			}
			m_NB = 8;
			return;
		default:
			Assert(0, Util::CParamException());
		}
		COMP_CATCHTHIS
	}
	//------------------------------------------------------------------------
	void CImage::ResetState(void)
	{
		COMP_TRYTHIS
		m_index = 0;
		m_cur_L = 0;
		m_cur_C = 0;
		m_cur_RI = 0;
		COMP_CATCHTHIS
	}
	//------------------------------------------------------------------------
} // end namespace COMP
