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
//	FileName:		CWBlock.cpp
//
//	Date Created:	07/05/1999
//
//	Author:			Youssef Ouaghli
//
//	Description:	CWBlock class definition.
//
//	Last Modified:	$Dmae: 1999/05/21 12:53:00 $
//
//  RCS Id: $Id: CWBlock.cpp,v 1.6 1999/05/21 12:53:00 xne Exp $
//
//  $Rma: CWBlock.cpp,v $
//  Revision 1.6  1999/05/21 12:53:00  xne
//  Introduced checks wrt the size of the blocks on which the S+P iterations
//  are to be performed.
//  Removed UpdateDC and the corresponding inverse methods.
//
//  Revision 1.5  1999/05/18 12:17:38  xne
//  Removed some #ifdef _DEBUG around Asserts since the impact on performances
//  is minimal
//
//  Revision 1.4  1999/05/10 12:04:59  youag
//  Testing if AC interval rescaling is needed before calling the function
//
//  Revision 1.3  1999/05/10 11:40:02  youag
//  Cleaning code from unused member functions
//
//  Revision 1.2  1999/05/07 14:42:03  youag
//  Tuning performance for ALPHA
//
//  Revision 1.1  1999/05/07 14:10:22  youag
//  Reduce number of inline functions in order to promote code expansion of
//  important inline functions
//
//
//////////////////////////////////////////////////////////////////////////
//:End Ignore

#include <math.h>

#include "CWBlock.h"

namespace COMP
{

	void CWBlock::St1DH_Fwd(const unsigned int i_I, const unsigned int i_S)
	{
		COMP_TRYTHIS_SPEED
		const unsigned int sd2 = i_S >> 1;
		int *pL = m_Data[i_I] + sd2;
		int *pH = pL + sd2;
		if (sd2 > 1)
		{
			int *pB = &m_Tmp[0];
			pL -= sd2;
			unsigned int n;
			for (n = i_S; n; n--)
				*pB++ = *pL++;
			pL -= sd2;
			for (n = sd2; n; n--)
			{
				const int c1 = *--pB;
				const int c0 = *--pB;
				*--pL = (c0 + c1) >> 1;
				*--pH = c0 - c1;
			}
		}
		else
		{
			if (sd2 == 1)
			{
				const int c1 = *--pH;
				const int c0 = *--pL;
				*pL = (c0 + c1) >> 1;
				*pH = c0 - c1;
			}
		}
		COMP_CATCHTHIS_SPEED
	}

	void CWBlock::St1DV_Fwd(const unsigned int i_I, const unsigned int i_S)
	{
		COMP_TRYTHIS_SPEED
		const unsigned int sd2 = i_S >> 1;
		int **pL = &m_Data[sd2];
		int **pH = pL + sd2;

		if (sd2 > 1)
		{
			int *pB = &m_Tmp[0];
			pL -= sd2;
			unsigned int n;
			for (n = i_S; n; n--)
				*pB++ = (*pL++)[i_I];
			pL -= sd2;
			for (n = sd2; n; n--)
			{
				const int c1 = *--pB;
				const int c0 = *--pB;
				(*--pL)[i_I] = (c0 + c1) >> 1;
				(*--pH)[i_I] = c0 - c1;
			}
		}
		else
		{
			if (sd2 == 1)
			{
				const int c1 = (*--pH)[i_I];
				const int c0 = (*--pL)[i_I];
				(*pL)[i_I] = (c0 + c1) >> 1;
				(*pH)[i_I] = c0 - c1;
			}
		}
		COMP_CATCHTHIS_SPEED
	}

	void CWBlock::St1DH_Inv(const unsigned int i_I, const unsigned int i_S)
	{
		COMP_TRYTHIS_SPEED
		const unsigned int sd2 = i_S >> 1;
		int *pL = m_Data[i_I] + sd2;
		int *pH = pL + sd2;
		if (sd2 > 1)
		{
			int *pB = &m_Tmp[i_S];
			unsigned int n;
			for (n = sd2; n; n--)
			{
				const int h0 = *--pH;
				const int c0 = *--pL + ((h0 + 1) >> 1);
				*--pB = c0 - h0;
				*--pB = c0;
			}

			for (n = i_S; n; n--)
				*pL++ = *pB++;
		}
		else
		{
			if (sd2 == 1)
			{
				const int h0 = *--pH;
				const int c0 = *--pL + ((h0 + 1) >> 1);
				*pH = c0 - h0;
				*pL = c0;
			}
		}
		COMP_CATCHTHIS_SPEED
	}

	void CWBlock::St1DV_Inv(const unsigned int i_I, const unsigned int i_S)
	{
		COMP_TRYTHIS_SPEED
		const unsigned int sd2 = i_S >> 1;
		int **pL = &m_Data[sd2];
		int **pH = pL + sd2;
		if (sd2 > 1)
		{
			int *pB = &m_Tmp[i_S];
			unsigned int n;
			for (n = sd2; n; n--)
			{
				const int h0 = (*--pH)[i_I];
				const int c0 = (*--pL)[i_I] + ((h0 + 1) >> 1);
				*--pB = c0 - h0;
				*--pB = c0;
			}
			for (n = i_S; n; n--)
				(*pL++)[i_I] = *pB++;
		}
		else
		{
			if (sd2 == 1)
			{
				const int h0 = (*--pH)[i_I];
				const int c0 = (*--pL)[i_I] + ((h0 + 1) >> 1);
				(*pH)[i_I] = c0 - h0;
				(*pL)[i_I] = c0;
			}
		}
		COMP_CATCHTHIS_SPEED
	}

	void CWBlock::SptA1DH_Fwd(const unsigned int i_I, const unsigned int i_S)
	{
		COMP_TRYTHIS_SPEED
		const unsigned int sd2 = i_S >> 1;
		int *pL = m_Data[i_I] + sd2;
		int *pH = pL + sd2;

		if (sd2 > 1)
		{
			int *pB = &m_Tmp[0];
			pL -= sd2;
			unsigned int n;
			for (n = i_S; n; n--)
				*pB++ = *pL++;
			pL -= sd2;
			int c1 = *--pB;
			int c0 = *--pB;
			int l1 = *--pL = (c0 + c1) >> 1;
			int h0 = c0 - c1;
			c1 = *--pB;
			c0 = *--pB;
			int l0 = *--pL = (c0 + c1) >> 1;
			int dl0 = l0 - l1;
			*--pH = h0 - ((dl0 + 2) >> 2);
			h0 = c0 - c1;
			for (n = sd2 - 2; n; n--)
			{
				c1 = *--pB;
				c0 = *--pB;
				l1 = l0;
				l0 = *--pL = (c0 + c1) >> 1;
				const int dl1 = dl0;
				dl0 = l0 - l1;
				*--pH = h0 - ((dl0 + dl1 + 2) >> 2);
				h0 = c0 - c1;
			}
			*--pH = h0 - ((dl0 + 2) >> 2);
		}
		else if (sd2 == 1)
		{
			const int c1 = *--pH;
			const int c0 = *--pL;
			*pL = (c0 + c1) >> 1;
			*pH = c0 - c1;
		}
		COMP_CATCHTHIS_SPEED
	}

	void CWBlock::SptA1DV_Fwd(const unsigned int i_I, const unsigned int i_S)
	{
		COMP_TRYTHIS_SPEED
		const unsigned int sd2 = i_S >> 1;
		int **pL = &m_Data[sd2];
		int **pH = pL + sd2;

		if (sd2 > 1)
		{
			int *pB = &m_Tmp[0];
			pL -= sd2;
			unsigned int n;
			for (n = i_S; n; n--)
				*pB++ = (*pL++)[i_I];
			pL -= sd2;
			int c1 = *--pB;
			int c0 = *--pB;
			int l1 = (*--pL)[i_I] = (c0 + c1) >> 1;
			int h0 = c0 - c1;
			c1 = *--pB;
			c0 = *--pB;
			int l0 = (*--pL)[i_I] = (c0 + c1) >> 1;
			int dl0 = l0 - l1;
			(*--pH)[i_I] = h0 - ((dl0 + 2) >> 2);
			h0 = c0 - c1;
			for (n = sd2 - 2; n; n--)
			{
				c1 = *--pB;
				c0 = *--pB;
				l1 = l0;
				l0 = (*--pL)[i_I] = (c0 + c1) >> 1;
				const int dl1 = dl0;
				dl0 = l0 - l1;
				(*--pH)[i_I] = h0 - ((dl0 + dl1 + 2) >> 2);
				h0 = c0 - c1;
			}
			(*--pH)[i_I] = h0 - ((dl0 + 2) >> 2);
		}
		else if (sd2 == 1)
		{
			const int c1 = (*--pH)[i_I];
			const int c0 = (*--pL)[i_I];
			(*pL)[i_I] = (c0 + c1) >> 1;
			(*pH)[i_I] = c0 - c1;
		}
		COMP_CATCHTHIS_SPEED
	}

	void CWBlock::SptA1DH_Inv(const unsigned int i_I, const unsigned int i_S)
	{
		COMP_TRYTHIS_SPEED
		const unsigned int sd2 = i_S >> 1;
		int *pL = m_Data[i_I] + sd2;
		int *pH = pL + sd2;
		if (sd2 > 1)
		{
			int *pB = &m_Tmp[i_S];
			int l1 = *--pL;
			int l0 = *--pL;
			int dl0 = l0 - l1;
			int h1 = *--pH + ((dl0 + 2) >> 2);
			int c0 = l1 + ((h1 + 1) >> 1);
			*--pB = c0 - h1;
			*--pB = c0;
			unsigned int n;
			for (n = sd2 - 2; n; n--)
			{
				l1 = l0;
				l0 = *--pL;
				const int dl1 = dl0;
				dl0 = l0 - l1;
				h1 = *--pH + ((dl0 + dl1 + 2) >> 2);
				c0 = l1 + ((h1 + 1) >> 1);
				*--pB = c0 - h1;
				*--pB = c0;
			}
			h1 = *--pH + ((dl0 + 2) >> 2);
			c0 = l0 + ((h1 + 1) >> 1);
			*--pB = c0 - h1;
			*--pB = c0;
			for (n = i_S; n; n--)
				*pL++ = *pB++;
		}
		else if (sd2 == 1)
		{
			const int h0 = *--pH;
			const int c0 = *--pL + ((h0 + 1) >> 1);
			*pL = c0;
			*pH = c0 - h0;
		}
		COMP_CATCHTHIS_SPEED
	}

	void CWBlock::SptA1DV_Inv(const unsigned int i_I, const unsigned int i_S)
	{
		COMP_TRYTHIS_SPEED
		const unsigned int sd2 = i_S >> 1;
		int **pL = &m_Data[sd2];
		int **pH = pL + sd2;
		if (sd2 > 1)
		{
			int *pB = &m_Tmp[i_S];
			int l1 = (*--pL)[i_I];
			int l0 = (*--pL)[i_I];
			int dl0 = l0 - l1;
			int h1 = (*--pH)[i_I] + ((dl0 + 2) >> 2);
			int c0 = l1 + ((h1 + 1) >> 1);
			*--pB = c0 - h1;
			*--pB = c0;
			unsigned int n;
			for (n = sd2 - 2; n; n--)
			{
				l1 = l0;
				l0 = (*--pL)[i_I];
				const int dl1 = dl0;
				dl0 = l0 - l1;
				h1 = (*--pH)[i_I] + ((dl0 + dl1 + 2) >> 2);
				c0 = l1 + ((h1 + 1) >> 1);
				*--pB = c0 - h1;
				*--pB = c0;
			}
			h1 = (*--pH)[i_I] + ((dl0 + 2) >> 2);
			c0 = l0 + ((h1 + 1) >> 1);
			*--pB = c0 - h1;
			*--pB = c0;
			for (n = i_S; n; n--)
				(*pL++)[i_I] = *pB++;
		}
		else if (sd2 == 1)
		{
			const int h0 = (*--pH)[i_I];
			const int c0 = (*--pL)[i_I] + ((h0 + 1) >> 1);
			(*pL)[i_I] = c0;
			(*pH)[i_I] = c0 - h0;
		}
		COMP_CATCHTHIS_SPEED
	}

	void CWBlock::SptB1DH_Fwd(const unsigned int i_I, const unsigned int i_S)
	{
		COMP_TRYTHIS_SPEED
		const unsigned int sd2 = i_S >> 1;
		int *pL = m_Data[i_I] + sd2;
		int *pH = pL + sd2;

		if (sd2 > 1)
		{
			int *pB = &m_Tmp[0];
			pL -= sd2;
			unsigned int n;
			for (n = i_S; n; n--)
				*pB++ = *pL++;
			pL -= sd2;
			int c1 = *--pB;
			int c0 = *--pB;
			int l1 = *--pL = (c0 + c1) >> 1;
			int h1 = c0 - c1;
			c1 = *--pB;
			c0 = *--pB;
			int l0 = *--pL = (c0 + c1) >> 1;
			int h0 = c0 - c1;
			int dl0 = l0 - l1;
			*--pH = h1 - ((dl0 + 2) >> 2);
			for (n = sd2 - 2; n; n--)
			{
				c1 = *--pB;
				c0 = *--pB;
				l1 = l0;
				l0 = *--pL = (c0 + c1) >> 1;
				const int dl1 = dl0;
				dl0 = l0 - l1;
				*--pH = h0 - ((((dl0 + dl1 - h1) << 1) + dl1 + 4) >> 3);
				h1 = h0;
				h0 = c0 - c1;
			}
			*--pH = h0 - ((dl0 + 2) >> 2);
		}
		else if (sd2 == 1)
		{
			const int c1 = *--pH;
			const int c0 = *--pL;
			*pL = (c0 + c1) >> 1;
			*pH = c0 - c1;
		}
		COMP_CATCHTHIS_SPEED
	}

	void CWBlock::SptB1DV_Fwd(const unsigned int i_I, const unsigned int i_S)
	{
		COMP_TRYTHIS_SPEED
		const unsigned int sd2 = i_S >> 1;
		int **pL = &m_Data[sd2];
		int **pH = pL + sd2;

		if (sd2 > 1)
		{
			int *pB = &m_Tmp[0];
			pL -= sd2;
			unsigned int n;
			for (n = i_S; n; n--)
				*pB++ = (*pL++)[i_I];
			pL -= sd2;
			int c1 = *--pB;
			int c0 = *--pB;
			int l1 = (*--pL)[i_I] = (c0 + c1) >> 1;
			int h1 = c0 - c1;
			c1 = *--pB;
			c0 = *--pB;
			int l0 = (*--pL)[i_I] = (c0 + c1) >> 1;
			int h0 = c0 - c1;
			int dl0 = l0 - l1;
			(*--pH)[i_I] = h1 - ((dl0 + 2) >> 2);
			for (n = sd2 - 2; n; n--)
			{
				c1 = *--pB;
				c0 = *--pB;
				l1 = l0;
				l0 = (*--pL)[i_I] = (c0 + c1) >> 1;
				const int dl1 = dl0;
				dl0 = l0 - l1;
				(*--pH)[i_I] = h0 - ((((dl0 + dl1 - h1) << 1) + dl1 + 4) >> 3);
				h1 = h0;
				h0 = c0 - c1;
			}
			(*--pH)[i_I] = h0 - ((dl0 + 2) >> 2);
		}
		else if (sd2 == 1)
		{
			const int c1 = (*--pH)[i_I];
			const int c0 = (*--pL)[i_I];
			(*pL)[i_I] = (c0 + c1) >> 1;
			(*pH)[i_I] = c0 - c1;
		}
		COMP_CATCHTHIS_SPEED
	}

	void CWBlock::SptB1DH_Inv(const unsigned int i_I, const unsigned int i_S)
	{
		COMP_TRYTHIS_SPEED
		const unsigned int sd2 = i_S >> 1;
		int *pL = m_Data[i_I] + sd2;
		int *pH = pL + sd2;
		if (sd2 > 1)
		{
			int *pB = &m_Tmp[i_S];
			int l1 = *--pL;
			int l0 = *--pL;
			int dl0 = l0 - l1;
			int h1 = *--pH + ((dl0 + 2) >> 2);
			int c0 = l1 + ((h1 + 1) >> 1);
			*--pB = c0 - h1;
			*--pB = c0;
			unsigned int n;
			for (n = sd2 - 2; n; n--)
			{
				l1 = l0;
				l0 = *--pL;
				const int dl1 = dl0;
				dl0 = l0 - l1;
				h1 = *--pH + ((((dl0 + dl1 - h1) << 1) + dl1 + 4) >> 3);
				c0 = l1 + ((h1 + 1) >> 1);
				*--pB = c0 - h1;
				*--pB = c0;
			}
			h1 = *--pH + ((dl0 + 2) >> 2);
			c0 = l0 + ((h1 + 1) >> 1);
			*--pB = c0 - h1;
			*--pB = c0;
			for (n = i_S; n; n--)
				*pL++ = *pB++;
		}
		else if (sd2 == 1)
		{
			const int h0 = *--pH;
			const int c0 = *--pL + ((h0 + 1) >> 1);
			*pL = c0;
			*pH = c0 - h0;
		}
		COMP_CATCHTHIS_SPEED
	}

	void CWBlock::SptB1DV_Inv(const unsigned int i_I, const unsigned int i_S)
	{
		COMP_TRYTHIS_SPEED
		const unsigned int sd2 = i_S >> 1;
		int **pL = &m_Data[sd2];
		int **pH = pL + sd2;
		if (sd2 > 1)
		{
			int *pB = &m_Tmp[i_S];
			int l1 = (*--pL)[i_I];
			int l0 = (*--pL)[i_I];
			int dl0 = l0 - l1;
			int h1 = (*--pH)[i_I] + ((dl0 + 2) >> 2);
			int c0 = l1 + ((h1 + 1) >> 1);
			*--pB = c0 - h1;
			*--pB = c0;
			unsigned int n;
			for (n = sd2 - 2; n; n--)
			{
				l1 = l0;
				l0 = (*--pL)[i_I];
				const int dl1 = dl0;
				dl0 = l0 - l1;
				h1 = (*--pH)[i_I] + ((((dl0 + dl1 - h1) << 1) + dl1 + 4) >> 3);
				c0 = l1 + ((h1 + 1) >> 1);
				*--pB = c0 - h1;
				*--pB = c0;
			}
			h1 = (*--pH)[i_I] + ((dl0 + 2) >> 2);
			c0 = l0 + ((h1 + 1) >> 1);
			*--pB = c0 - h1;
			*--pB = c0;
			for (n = i_S; n; n--)
				(*pL++)[i_I] = *pB++;
		}
		else if (sd2 == 1)
		{
			const int h0 = (*--pH)[i_I];
			const int c0 = (*--pL)[i_I] + ((h0 + 1) >> 1);
			(*pL)[i_I] = c0;
			(*pH)[i_I] = c0 - h0;
		}
		COMP_CATCHTHIS_SPEED
	}

	void CWBlock::SptC1DH_Fwd(const unsigned int i_I, const unsigned int i_S)
	{
		COMP_TRYTHIS_SPEED
		if (i_S > 2)
		{
			const unsigned int sd2 = i_S >> 1;
			int *pL = m_Data[i_I];
			int *pH = pL + sd2;

			int l0 = *pL++;
			int l1 = *pL++;
			int dl2 = l0 - l1;
			*pH++ -= (dl2 + 2) >> 2;
			if (sd2 > 2)
			{
				l0 = l1;
				l1 = *pL++;
				int dl1 = dl2;
				dl2 = l0 - l1;
				*pH -= (((dl1 + dl2 - pH[1]) << 1) + dl2 + 4) >> 3;
				pH++;
				for (unsigned int n = sd2 - 3; n; n--)
				{
					l0 = l1;
					l1 = *pL++;
					const int dl0 = dl1;
					dl1 = dl2;
					dl2 = l0 - l1;
					*pH -= (-dl0 + ((((dl1 + (dl2 << 1) - pH[1]) << 1) - pH[1]) << 1) + 8) >> 4;
					pH++;
				}
			}
			*pH++ -= (dl2 + 2) >> 2;
		}
		COMP_CATCHTHIS_SPEED
	}

	void CWBlock::SptC1DV_Fwd(const unsigned int i_I, const unsigned int i_S)
	{
		COMP_TRYTHIS_SPEED
		if (i_S > 2)
		{
			const unsigned int sd2 = i_S >> 1;
			int **pL = &m_Data[0];
			int **pH = pL + sd2;

			int l0 = (*pL++)[i_I];
			int l1 = (*pL++)[i_I];
			int dl2 = l0 - l1;
			(*pH++)[i_I] -= (dl2 + 2) >> 2;
			if (sd2 > 2)
			{
				l0 = l1;
				l1 = (*pL++)[i_I];
				int dl1 = dl2;
				dl2 = l0 - l1;
				(*pH)[i_I] -= (((dl1 + dl2 - pH[1][i_I]) << 1) + dl2 + 4) >> 3;
				pH++;
				for (unsigned int n = sd2 - 3; n; n--)
				{
					l0 = l1;
					l1 = (*pL++)[i_I];
					const int dl0 = dl1;
					dl1 = dl2;
					dl2 = l0 - l1;
					(*pH)[i_I] -= (-dl0 + ((((dl1 + (dl2 << 1) - pH[1][i_I]) << 1) - pH[1][i_I]) << 1) + 8) >> 4;
					pH++;
				}
			}
			(*pH++)[i_I] -= (dl2 + 2) >> 2;
		}
		COMP_CATCHTHIS_SPEED
	}

	void CWBlock::SptC1DH_Inv(const unsigned int i_I, const unsigned int i_S)
	{
		COMP_TRYTHIS_SPEED
		if (i_S > 2)
		{
			const unsigned int sd2 = i_S >> 1;
			int *pL = m_Data[i_I] + sd2;
			int *pH = pL + sd2;

			int l1 = *--pL;
			int l0 = *--pL;
			int dl0 = l0 - l1;
			*--pH += (dl0 + 2) >> 2;
			if (sd2 > 2)
			{
				l1 = l0;
				l0 = *--pL;
				int dl1 = dl0;
				dl0 = l0 - l1;
				for (unsigned int n = sd2 - 3; n; n--)
				{
					l1 = l0;
					l0 = *--pL;
					const int dl2 = dl1;
					dl1 = dl0;
					dl0 = l0 - l1;
					pH--;
					*pH += (-dl0 + ((((dl1 + (dl2 << 1) - pH[1]) << 1) - pH[1]) << 1) + 8) >> 4;
				}
				pH--;
				*pH += (((dl0 + dl1 - pH[1]) << 1) + dl1 + 4) >> 3;
			}
			*--pH += (dl0 + 2) >> 2;
		}
		COMP_CATCHTHIS_SPEED
	}

	void CWBlock::SptC1DV_Inv(const unsigned int i_I, const unsigned int i_S)
	{
		COMP_TRYTHIS_SPEED
		if (i_S > 2)
		{
			const unsigned int sd2 = i_S >> 1;
			int **pL = &m_Data[sd2];
			int **pH = pL + sd2;

			int l1 = (*--pL)[i_I];
			int l0 = (*--pL)[i_I];
			int dl0 = l0 - l1;
			(*--pH)[i_I] += (dl0 + 2) >> 2;
			if (sd2 > 2)
			{
				l1 = l0;
				l0 = (*--pL)[i_I];
				int dl1 = dl0;
				dl0 = l0 - l1;
				for (unsigned int n = sd2 - 3; n; n--)
				{
					l1 = l0;
					l0 = (*--pL)[i_I];
					const int dl2 = dl1;
					dl1 = dl0;
					dl0 = l0 - l1;
					pH--;
					(*pH)[i_I] += (-dl0 + ((((dl1 + (dl2 << 1) - pH[1][i_I]) << 1) - pH[1][i_I]) << 1) + 8) >> 4;
				}
				pH--;
				(*pH)[i_I] += (((dl0 + dl1 - pH[1][i_I]) << 1) + dl1 + 4) >> 3;
			}
			(*--pH)[i_I] += (dl0 + 2) >> 2;
		}
		COMP_CATCHTHIS_SPEED
	}

	void CWBlock::Resize(const unsigned int i_W, const unsigned int i_H)
	{
		COMP_TRYTHIS
		if (m_W != i_W || m_H != i_H)
		{
			m_W = i_W;
			m_H = i_H;
			m_Data.clear();
			m_Buffer.clear();
			m_Tmp.clear();
			m_Size = (unsigned long)m_W * m_H;
			if (m_Size)
			{
				m_Data = std::vector<int *>(m_H, NULL);
				m_Buffer = std::vector<int>(m_Size, 0);
				for (unsigned int i = 0; i < m_H; i++)
					m_Data[i] = &m_Buffer[i * m_W];
				m_Tmp = std::vector<int>(m_W > m_H ? m_W : m_H, 0);
			}
		}
		COMP_CATCHTHIS
	}

	void CWBlock::GetAndPad(CImage &i_Img,
							const unsigned int i_X, const unsigned int i_Y,
							const unsigned int i_W, const unsigned int i_H)
	{
		COMP_TRYTHIS
		Assert(i_W <= m_W && i_H <= m_H, Util::CParamException());
		int *p = &m_Buffer[0];
		unsigned int i, y;
		for (i = 0, y = i_Y; i < i_H; i++, y++)
		{
			const unsigned short *pI = i_Img.GetP()[y] + i_X;
			unsigned int j;
			for (j = 0; j < i_W; j++)
				*p++ = *pI++;
			if (j < m_W)
				for (int c = p[-1]; j < m_W; j++)
					*p++ = c;
		}
		for (; i < m_H; i++)
			memcpy((char *)m_Data[i], (char *)m_Data[i - 1], sizeof(int) * m_W);
		COMP_CATCHTHIS
	}

	void CWBlock::Put(CImage &o_Img,
					  const unsigned int i_X, const unsigned int i_Y,
					  const unsigned int i_W, const unsigned int i_H)
	{
		COMP_TRYTHIS
		Assert(i_W <= m_W && i_H <= m_H, Util::CParamException());
		const int maxCoef = (1L << o_Img.GetNB()) - 1;
		for (unsigned int i = 0, y = i_Y; i < i_H; i++, y++)
		{
			const int *p = m_Data[i];
			unsigned short *pO = o_Img.GetP()[y] + i_X;
			for (unsigned int j = i_W; j; j--)
			{
				const int c = *p++;
				*pO++ = c < 0 ? 0 : c > maxCoef ? maxCoef
												: c;
			}
		}
		COMP_CATCHTHIS
	}

	void CWBlock::IterateSt(const bool i_Forward, const unsigned int i_NbIte)
	{
		COMP_TRYTHIS
		if (i_Forward)
			for (unsigned int ite = 0; ite < i_NbIte; ite++)
				St2D(true, m_W >> ite, m_H >> ite);
		else
			for (unsigned int ite = i_NbIte; ite > 0; ite--)
				St2D(false, m_W >> (ite - 1), m_H >> (ite - 1));
		COMP_CATCHTHIS
	}

	void CWBlock::IterateSptA(const bool i_Forward, const unsigned int i_NbIte)
	{
		COMP_TRYTHIS
		if (i_Forward)
			for (unsigned int ite = 0; ite < i_NbIte; ite++)
				SptA2D(true, m_W >> ite, m_H >> ite);
		else
			for (unsigned int ite = i_NbIte; ite; ite--)
				SptA2D(false, m_W >> (ite - 1), m_H >> (ite - 1));
		COMP_CATCHTHIS
	}

	void CWBlock::IterateSptB(const bool i_Forward, const unsigned int i_NbIte)
	{
		COMP_TRYTHIS
		if (i_Forward)
			for (unsigned int ite = 0; ite < i_NbIte; ite++)
				SptB2D(true, m_W >> ite, m_H >> ite);
		else
			for (unsigned int ite = i_NbIte; ite; ite--)
				SptB2D(false, m_W >> (ite - 1), m_H >> (ite - 1));
		COMP_CATCHTHIS
	}

	void CWBlock::IterateSptC(const bool i_Forward, const unsigned int i_NbIte)
	{
		COMP_TRYTHIS
		if (i_Forward)
			for (unsigned int ite = 0; ite < i_NbIte; ite++)
				SptC2D(true, m_W >> ite, m_H >> ite);
		else
			for (unsigned int ite = i_NbIte; ite > 0; ite--)
				SptC2D(false, m_W >> (ite - 1), m_H >> (ite - 1));
		COMP_CATCHTHIS
	}

	int CWBlock::GetMaxCoef(void)
		const
	{
		COMP_TRYTHIS
		int maxC = 0L;
		int minC = 0L;
		const int *p = &m_Buffer[0];
		for (unsigned long i = m_Size; i; i--)
		{
			const int c = *p++;
			if (c > maxC)
				maxC = c;
			else if (c < minC)
				minC = c;
		}
		if (maxC < -minC)
			maxC = -minC;
		return maxC;
		COMP_CATCHTHIS
	}

	int CWBlock::GetQuadrantMaxCoef(const unsigned int i_X, const unsigned int i_Y,
									const unsigned int i_W, const unsigned int i_H)
		const
	{
		COMP_TRYTHIS_SPEED
		Assert((i_X + i_W) <= m_W && (i_Y + i_H) <= m_H, Util::CParamException());
		const unsigned int endY = i_Y + i_H;
		int maxC = 0L;
		int minC = 0L;
		for (unsigned int i = i_Y; i < endY; i++)
		{
			const int *p = m_Data[i] + i_X;
			for (unsigned int j = i_W; j; j--)
			{
				const int c = *p++;
				if (c > maxC)
					maxC = c;
				else if (c < minC)
					minC = c;
			}
		}
		if (maxC < -minC)
			maxC = -minC;
		return maxC;
		COMP_CATCHTHIS_SPEED
	}

} // end namespace
