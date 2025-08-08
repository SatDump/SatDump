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
//	FileName:		CVLCDecoder.cpp
//
//	Date Created:	15/01/1999
//
//	Author:			Youssef Ouaghli
//
//	Description:	CVLCDecoder class definition.
//
//	Last Modified:	$Dmae: 1999/08/26 15:41:08 $
//
//  RCS Id: $Id: CVLCDecoder.cpp,v 1.47 1999/08/26 15:41:08 youag Exp $
//
//  $Rma: CVLCDecoder.cpp,v $
//  Revision 1.47  1999/08/26 15:41:08  youag
//  New context: (PrevContext + PrevSymbol) / 2 instead of PrevSymbol
//
//  Revision 1.46  1999/06/01 14:30:48  youag
//  Small change
//
//  Revision 1.45  1999/05/27 11:05:59  youag
//  Removing un-needed parameters in CodeQuadrantDC()/DecodeQuadrantDC functions
//
//  Revision 1.44  1999/05/21 15:34:35  youag
//  Replacing speed_nb_bits() by speed_csize()
//
//  Revision 1.43  1999/05/21 14:45:01  youag
//  CVLCCoder & CVLCDecoder bad param testing done
//
//  Revision 1.42  1999/05/21 10:28:39  youag
//  Computing/Getting WT bloc max coef inside CVLCCoder::Code()/CVLCDecoder::Decode()
//  to avoid possible trap with the interface
//
//  Revision 1.41  1999/05/20 13:59:21  youag
//  Coding DC quadrant coefs using DPCM
//
//  Revision 1.40  1999/05/20 13:19:43  youag
//  Correcting coding/decoding of DC quadrant when nbBit is zero
//
//  Revision 1.39  1999/05/20 13:03:29  youag
//  Coding/Decoding the DC quadrant as a separate case
//
//  Revision 1.38  1999/05/20 12:28:19  youag
//  Added Assert in CVLCCoder::CodeQuadrant for better testing of the class
//
//  Revision 1.37  1999/05/18 12:20:20  youag
//  Renaming ac, m_Ac into ad, m_Ad for CACDecoder class
//
//  Revision 1.36  1999/05/11 16:08:51  youag
//  Removing CVLCCodec::m_Mod optimization beacause of strange effect
//  during decompression on ALPHA (execution time is doubled)
//
//  Revision 1.35  1999/05/11 15:54:54  youag
//  CVLCCodec::m_Mod is now an array of pointers. to minimize number of
//  assembly lines needed to initialize parameter of CACCoder:;CodeSymbol function
//
//  Revision 1.34  1999/05/10 12:04:58  youag
//  Testing if AC interval rescaling is needed before calling the function
//
//  Revision 1.33  1999/05/10 11:40:01  youag
//  Cleaning code from unused member functions
//
//  Revision 1.32  1999/05/07 14:08:32  youag
//  Reduce number of inline functions in order to promote code expansion of
//  important inline functions
//
//  Revision 1.31  1999/05/07 10:53:43  youag
//  Checking for CWBuffer size expansion at each 4 bytes
//
//  Revision 1.30  1999/05/06 15:42:32  youag
//  Reducing use of const & param because of poor assembly code
//
//  Revision 1.29  1999/04/26 13:32:28  youag
//  Avoiding second test of coding the MPS by using two
//  CACModel::Update{Mps/Lps} functions
//
//  Revision 1.28  1999/04/20 16:27:39  youag
//  loop indexes test optimization
//
//  Revision 1.27  1999/04/20 10:47:01  youag
//  Testing some loop indexes against 0:
//  Replacing for (i=0 ; i<N ; i++) by for (i=N ; i!=0 ; i--)
//
//  Revision 1.26  1999/04/20 09:59:14  youag
//  Using *pointer++ optimization in CWBlock:: GetAndPad & Put methods
//
//  Revision 1.25  1999/04/16 11:52:20  youag
//  Using pointer instead of CWBlock::CGet(x, y) or CWBlock(x, y)
//  Performances are improved one ALPHA, equal on INTEL
//
//  Revision 1.24  1999/04/14 17:24:13  youag
//  Optimization
//
//  Revision 1.23  1999/04/14 14:12:52  youag
//  Improving CACDecoder performance and cleaning WTConst.h
//
//  Revision 1.22  1999/04/14 12:01:02  youag
//  Optimizing CWBlock
//  Resolving bug in S+P with predictor C and quadrant size == 4
//
//  Revision 1.21  1999/04/13 11:47:09  youag
//  Computing max WT coef per quadrant for improved CR
//
//  Revision 1.20  1999/04/12 17:56:44  youag
//  Optimization of AC
//  Avoiding 0xFF byte stuffing in WT header
//
//  Revision 1.19  1999/04/06 14:55:06  youag
//  Improving error detection
//
//  Revision 1.18  1999/03/29 15:31:24  youag
//  Resolving bug in coefs refinement after lossy decoding
//
//  Revision 1.17  1999/03/29 10:25:13  youag
//  Resolving bug in lossy mode
//
//  Revision 1.16  1999/03/25 16:44:33  youag
//  Small optimization for lossless case
//
//  Revision 1.15  1999/03/22 09:27:23  youag
//  Removing SPIHT modules
//
//  Revision 1.14  1999/03/05 17:24:02  youag
//  Implementing bit plane based lossy codec with VLC
//
//  Revision 1.13  1999/03/04 18:29:37  youag
//  Introducing bit plane based lossy codec
//
//  Revision 1.12  1999/03/01 12:35:35  youag
//  Better detection of errors during decoding.
//  nSPIHTLevels is now interpreted as the number of least significant bit
//  planes that must not be coded hierarchically.
//
//  Revision 1.11  1999/02/25 12:20:37  youag
//  Small changes and testing
//
//  Revision 1.10  1999/02/24 16:22:59  youag
//  Improving block coding by using variable nb bits per S+P coef instead of fixed one
//
//  Revision 1.9  1999/02/19 12:14:47  youag
//  SPIHT now uses group of four pixels / trees as in Said and Pearlman code
//
//  Revision 1.8  1999/02/09 17:31:59  youag
//  Small changes and tests
//
//  Revision 1.7  1999/02/08 18:09:09  youag
//  Handling Number of SPIHT levels parameter
//
//  Revision 1.6  1999/02/08 14:51:51  youag
//  VLC coding/decoding of last SPIHT bit planes operational
//
//  Revision 1.5  1999/02/05 17:56:29  youag
//  Added VLC coding for last SPIHT bit planes
//
//  Revision 1.4  1999/02/04 18:00:22  youag
//  Changes to make sources clearer
//
//  Revision 1.3  1999/01/29 11:45:13  youag
//  Modifying CBuffer for faster 1 bit read/write
//
//  Revision 1.2  1999/01/28 18:10:31  youag
//  Debuging VLC coding/decoding
//
//  Revision 1.1  1999/01/27 11:09:13  youag
//  Reduce use of inline functions
//
//
//////////////////////////////////////////////////////////////////////////
//:End Ignore

#include "CVLCDecoder.h"

namespace COMP
{

	bool CVLCDecoder::DecodeQuadrantDC(CWBlock &o_Wblk,
									   const unsigned int i_W, const unsigned int i_H,
									   const unsigned int /*i_Quadrant*/)
	{
		COMP_TRYTHIS
		unsigned int nbBit = m_Ad.DecodeBits(m_NbBitNbBitCoef);
		if (nbBit > m_NbBitCoef)
			return false;
		if (nbBit)
		{
			m_Mod = m_Models[nbBit++];
			if (!m_Mod[0].IsInitialized())
				for (unsigned int i = 0; i <= nbBit; i++)
					m_Mod[i].Initialize(nbBit + 1);

			int oldCoef = 1UL << (nbBit - 2);
			unsigned int context = nbBit;
			int *p = o_Wblk.GetData()[0];
			const unsigned int w = o_Wblk.GetW();
			for (int i = i_H; i > 0; i--, p += w)
			{
				for (unsigned int j = i_W; j; j--)
				{
					int c;

					context = (context + DecodeCoef(context, c)) >> 1;
					*p++ = (oldCoef += c);
				}
				if (--i)
				{
					p += w;
					for (unsigned int j = i_W; j; j--)
					{
						int c;

						context = (context + DecodeCoef(context, c)) >> 1;
						*--p = (oldCoef += c);
					}
				}
			}
		}
		else
		{
			for (int i = i_H - 1; i >= 0; i--)
			{
				int *p = o_Wblk.GetData()[i];
				for (unsigned int j = i_W; j; j--)
					*p++ = 0;
			}
		}
		return true;
		COMP_CATCHTHIS
	}

	bool CVLCDecoder::DecodeQuadrant(CWBlock &o_Wblk,
									 const unsigned int i_X, const unsigned int i_Y,
									 const unsigned int i_W, const unsigned int i_H,
									 const unsigned int i_Level, const unsigned int i_Quadrant)
	{
		COMP_TRYTHIS
		unsigned int nbBit = m_Ad.DecodeBits(m_NbBitNbBitCoef);
		if (nbBit > m_NbBitCoef)
			return false;
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
			int *p = o_Wblk.GetData()[i_Y] + i_X;
			const unsigned int w = o_Wblk.GetW();
			if (!coefShift)
				for (int i = i_H; i > 0; i--, p += w)
				{
					for (unsigned int j = i_W; j; j--)
						context = (context + DecodeCoef(context, *p++)) >> 1;
					if (--i)
					{
						p += w;
						for (unsigned int j = i_W; j; j--)
							context = (context + DecodeCoef(context, *--p)) >> 1;
					}
				}
			else
				for (int i = i_H; i > 0; i--, p += w)
				{
					for (unsigned int j = i_W; j; j--)
					{
						int c;

						context = (context + DecodeCoef(context, c)) >> 1;
						*p++ = c << coefShift;
					}
					if (--i)
					{
						p += w;
						for (unsigned int j = i_W; j; j--)
						{
							int c;

							context = (context + DecodeCoef(context, c)) >> 1;
							*--p = c << coefShift;
						}
					}
				}
		}
		else
		{
			const unsigned int endY = i_Y + i_H;
			for (unsigned int i = i_Y; i < endY; i++)
			{
				int *p = o_Wblk.GetData()[i] + i_X;
				for (unsigned int j = i_W; j; j--)
					*p++ = 0;
			}
		}
		return true;
		COMP_CATCHTHIS
	}

	void CVLCDecoder::RefineLossyQuadrant(CWBlock &o_Wblk,
										  const unsigned int i_X, const unsigned int i_Y,
										  const unsigned int i_W, const unsigned int i_H,
										  const unsigned int i_Level, const unsigned int i_Quadrant)
	{
		COMP_TRYTHIS_SPEED
		if (m_NbLossyBitPlane > (i_Level + 1 + (i_Quadrant > m_NbLossyQuadrant ? 1 : 0)))
		{
			const int cT = (1L << (m_NbLossyBitPlane - i_Level -
								   (i_Quadrant > m_NbLossyQuadrant ? 2 : 1))) -
						   1;
			const unsigned int endY = i_Y + i_H;
			for (unsigned int i = i_Y; i < endY; i++)
			{
				int *p = o_Wblk.GetData()[i] + i_X;
				for (unsigned int j = i_W; j; j--, p++)
				{
					int c;

					if ((c = *p))
					{
						if (c > 0)
							*p |= cT;
						else
							*p = -(-c | cT);
					}
				}
			}
		}
		COMP_CATCHTHIS_SPEED
	}

	void CVLCDecoder::RefineLossy(CWBlock &o_Wblk)
	{
		COMP_TRYTHIS
		unsigned int w = o_Wblk.GetW() >> m_NbIteWt;
		unsigned int h = o_Wblk.GetH() >> m_NbIteWt;
		unsigned int m = m_NbIteWt;
		unsigned int q = m_NbIteWt * 3 - 1;

		for (unsigned int k = 0; k < m_NbIteWt; k++, w <<= 1, h <<= 1, m--)
		{
			RefineLossyQuadrant(o_Wblk, w, 0, w, h, m, q--);
			RefineLossyQuadrant(o_Wblk, 0, h, w, h, m, q--);
			RefineLossyQuadrant(o_Wblk, w, h, w, h, m - 1, q--);
		}
		COMP_CATCHTHIS
	}

	bool CVLCDecoder::Decode(CWBlock &o_Wblk, const unsigned int i_NbIteWt, const unsigned int i_NbLossyBp)
	{
		COMP_TRYTHIS
		// Get the wavelet block max coef amplitude in number of bits
		const unsigned int nbBit = m_Ad.DecodeBits(5);
		// To make sure CACDecoder::DecodeBits() will not fail
		if (nbBit > (c_ACNbBits - 2))
			return false;
		if (nbBit)
		{
			static const unsigned int bitPlanes[16] = {0, 1, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4};
			static const unsigned int quadrants[16] = {0, 0, 0, 2, 3, 0, 2, 3, 5, 6, 0, 2, 3, 5, 6, 9};

			Assert(i_NbLossyBp < 16, Util::CParamException());
			unsigned int w = o_Wblk.GetW() >> i_NbIteWt;
			unsigned int h = o_Wblk.GetH() >> i_NbIteWt;
			// Check the block dimension against the number of wavelet transform iteration
			Assert((w << i_NbIteWt) == o_Wblk.GetW() || (h << i_NbIteWt) == o_Wblk.GetH(), Util::CParamException());
			unsigned int m = i_NbIteWt;
			unsigned int q = i_NbIteWt * 3;
			m_NbBitCoef = nbBit;
			m_NbBitNbBitCoef = speed_csize(nbBit);
			m_NbIteWt = i_NbIteWt;
			m_NbLossyBitPlane = bitPlanes[i_NbLossyBp];
			m_NbLossyQuadrant = quadrants[i_NbLossyBp];

			if (!DecodeQuadrantDC(o_Wblk, w, h, q--))
				return false;
			for (unsigned int k = 0; k < i_NbIteWt; k++, w <<= 1, h <<= 1, m--)
			{
				if (!DecodeQuadrant(o_Wblk, w, 0, w, h, m, q--))
					return false;
				if (!DecodeQuadrant(o_Wblk, 0, h, w, h, m, q--))
					return false;
				if (!DecodeQuadrant(o_Wblk, w, h, w, h, m - 1, q--))
					return false;
			}

			if (i_NbLossyBp > 1)
				RefineLossy(o_Wblk);
		}
		else
			o_Wblk.Zero(); // nbBit == 0

		return true;
		COMP_CATCHTHIS
	}

} // namespace COMP
