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
//	FileName:		CVLCCoder.cpp
//
//	Date Created:	15/01/1999
//
//	Author:			Youssef Ouaghli
//
//	Description:	CVLCCoder class definition.
//
//	Last Modified:	$Dmae: 1999/08/26 15:41:07 $
//
//  RCS Id: $Id: CVLCCoder.cpp,v 1.45 1999/08/26 15:41:07 youag Exp $
//
//////////////////////////////////////////////////////////////////////////
//:End Ignore

#include "CVLCCoder.h"

namespace COMP
{

	void CVLCCoder::CodeQuadrantDC(CWBlock &i_Wblk,
								   const unsigned int i_W, const unsigned int i_H,
								   const unsigned int /*i_Quadrant*/)
	{
		COMP_TRYTHIS
		// Get the number of bits needed to code the block coefs
		unsigned int nbBit = speed_csize(i_Wblk.GetQuadrantMaxCoef(0, 0, i_W, i_H));
		m_Ac.CodeBits(nbBit, m_NbBitNbBitCoef);
		if (nbBit)
		{
			m_Mod = m_Models[nbBit++];
			if (!m_Mod[0].IsInitialized())
				for (unsigned int i = 0; i <= nbBit; i++)
					m_Mod[i].Initialize(nbBit + 1);

			int oldCoef = 1UL << (nbBit - 2);
			unsigned int context = nbBit;
			const int *p = i_Wblk.GetData()[0];
			const unsigned int w = i_Wblk.GetW();
			for (int i = i_H; i > 0; i--, p += w)
			{
				for (unsigned int j = i_W; j; j--)
				{
					const int c = *p++;
					context = (context + CodeCoef(context, c - oldCoef)) >> 1;
					oldCoef = c;
				}
				if (--i)
				{
					p += w;
					for (unsigned int j = i_W; j; j--)
					{
						const int c = *--p;
						context = (context + CodeCoef(context, c - oldCoef)) >> 1;
						oldCoef = c;
					}
				}
			}
		}
		COMP_CATCHTHIS
	}

	void CVLCCoder::CodeQuadrant(CWBlock &i_Wblk,
								 const unsigned int i_X, const unsigned int i_Y,
								 const unsigned int i_W, const unsigned int i_H,
								 const unsigned int i_Level, const unsigned int i_Quadrant)
	{
		COMP_TRYTHIS
		// Get the number of bits needed to code the block coefs
		unsigned int nbBit = speed_csize(i_Wblk.GetQuadrantMaxCoef(i_X, i_Y, i_W, i_H));
		m_Ac.CodeBits(nbBit, m_NbBitNbBitCoef);
		const unsigned int coefShift = i_Level >= m_NbLossyBitPlane
										   ? 0
										   : m_NbLossyBitPlane - i_Level - (i_Quadrant > m_NbLossyQuadrant ? 1 : 0);
		if (nbBit > coefShift)
		{
			nbBit -= coefShift;
			m_Mod = m_Models[nbBit - 1];
			if (!m_Mod[0].IsInitialized())
				for (unsigned int i = 0; i <= nbBit; i++)
					m_Mod[i].Initialize(nbBit + 1);

			unsigned int context = nbBit;
			const int *p = i_Wblk.GetData()[i_Y] + i_X;
			const unsigned int w = i_Wblk.GetW();
			if (!coefShift)
				for (int i = i_H; i > 0; i--, p += w)
				{
					for (unsigned int j = i_W; j; j--)
						context = (context + CodeCoef(context, *p++)) >> 1;
					if (--i)
					{
						p += w;
						for (unsigned int j = i_W; j; j--)
							context = (context + CodeCoef(context, *--p)) >> 1;
					}
				}
			else
				for (int i = i_H; i > 0; i--, p += w)
				{
					for (unsigned int j = i_W; j; j--)
					{
						int cV = *p++;
						cV = cV >= 0 ? cV >> coefShift : -(-cV >> coefShift);
						context = (context + CodeCoef(context, cV)) >> 1;
					}
					if (--i)
					{
						p += w;
						for (unsigned int j = i_W; j; j--)
						{
							int cV = *--p;
							cV = cV >= 0 ? cV >> coefShift : -(-cV >> coefShift);
							context = (context + CodeCoef(context, cV)) >> 1;
						}
					}
				}
		}
		COMP_CATCHTHIS
	}

	void CVLCCoder::Code(CWBlock &i_Wblk, const unsigned int i_NbIteWt, const unsigned int i_NbLossyBp)
	{
		COMP_TRYTHIS
		// Get the number of bits needed to code the block coefs
		const unsigned int nbBit = speed_csize(i_Wblk.GetMaxCoef());
		// To make sure CACCoder::CodeBits() will not fail
		Assert(nbBit <= (c_ACNbBits - 2), Util::CParamException());
		m_Ac.CodeBits(nbBit, 5);
		if (nbBit)
		{
			static const unsigned int bitPlanes[16] = {0, 1, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4};
			static const unsigned int quadrants[16] = {0, 0, 0, 2, 3, 0, 2, 3, 5, 6, 0, 2, 3, 5, 6, 9};

			Assert(i_NbLossyBp < 16, Util::CParamException());
			unsigned int w = i_Wblk.GetW() >> i_NbIteWt;
			unsigned int h = i_Wblk.GetH() >> i_NbIteWt;
			// Check the block dimension against the number of wavelet transform iteration
			Assert((w << i_NbIteWt) == i_Wblk.GetW() || (h << i_NbIteWt) == i_Wblk.GetH(), Util::CParamException());
			unsigned int m = i_NbIteWt;
			unsigned int q = i_NbIteWt * 3;
			m_NbBitCoef = nbBit;
			m_NbBitNbBitCoef = speed_csize(nbBit);
			m_NbIteWt = i_NbIteWt;
			m_NbLossyBitPlane = bitPlanes[i_NbLossyBp];
			m_NbLossyQuadrant = quadrants[i_NbLossyBp];

			CodeQuadrantDC(i_Wblk, w, h, q--);
			for (unsigned int k = 0; k < i_NbIteWt; k++, w <<= 1, h <<= 1, m--)
			{
				CodeQuadrant(i_Wblk, w, 0, w, h, m, q--);
				CodeQuadrant(i_Wblk, 0, h, w, h, m, q--);
				CodeQuadrant(i_Wblk, w, h, w, h, m - 1, q--);
			}
		}
		COMP_CATCHTHIS
	}

} // namespace COMP
