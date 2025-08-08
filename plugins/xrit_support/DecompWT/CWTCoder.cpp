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

#include "CDataField.h"

#include "CBuffer.h"

#include "CWBlock.h"
#include "CACCoder.h"
#include "CVLCCoder.h"
#include "CWTCoder.h"

namespace COMP
{

	void CWTCoder::CodeBufferBlock(const unsigned int i_Bs)
	{
		COMP_TRYTHIS
		unsigned int bX, bY;

		// Write header
		m_Buf.write_marker(c_MarkerHEADER);
		m_Buf.real_write(m_Param.m_BitsPerPixel, 4);
		m_Buf.real_write(m_Image.GetW(), 16);
		m_Buf.real_write(m_Image.GetH(), 16);
		m_Buf.real_write(m_Param.m_nWTlevels - 3, 2);
		m_Buf.real_write(m_Param.m_PredMode - CWTParams::e_None, 2);
		m_Buf.real_write(i_Bs >> 5, 2);
		m_Buf.real_write(m_Param.m_RestartInterval, 16);
		m_Buf.real_write(m_Param.m_nLossyBitPlanes, 4);
		m_Buf.real_write(0, 2); // Pad bits
		m_Buf.write_marker(c_MarkerDATA);

		// Computing the number of blocks along each dimension
		unsigned int nbW = m_Image.GetW() / i_Bs;
		const unsigned int rW = m_Image.GetW() % i_Bs;
		if (rW)
			nbW++;
		unsigned int nbH = m_Image.GetH() / i_Bs;
		const unsigned int rH = m_Image.GetH() % i_Bs;
		if (rH)
			nbH++;
		Assert(nbW > 0 && nbH > 0, Util::CParamException());

		// Start arithmetic coder
		CACCoder ac(m_Buf);
		ac.Start();

		// VLC coder object creation
		CVLCCoder coder(ac);

		// Wavelet block object creation
		CWBlock wblk(i_Bs, i_Bs);

		// block loops
		unsigned int nbBlock = 0;
		unsigned int markerNum = 0;
		bool bACStopped = false;
		for (bY = 0; bY < nbH; bY++)
		{
			const unsigned int nH = bY == (nbH - 1) && rH ? rH : i_Bs;
			for (bX = 0; bX < nbW; bX++)
			{
				const unsigned int nW = bX == (nbW - 1) && rW ? rW : i_Bs;

				// Copy image block to CWBlock with line and column padding
				wblk.GetAndPad(m_Image, bX * i_Bs, bY * i_Bs, nW, nH);

				// Perform the forward m_nWTlevels iterations of 2D S(+P) transform
				switch (m_Param.m_PredMode)
				{
				case CWTParams::e_None:
					wblk.IterateSt(true, m_Param.m_nWTlevels);
					break;
				case CWTParams::e_PredA:
					wblk.IterateSptA(true, m_Param.m_nWTlevels);
					break;
				case CWTParams::e_PredB:
					wblk.IterateSptB(true, m_Param.m_nWTlevels);
					break;
				case CWTParams::e_PredC:
					wblk.IterateSptC(true, m_Param.m_nWTlevels);
					break;
				default:
					Assert(0, Util::CParamException())
				}

				// code the S+P coefs
				coder.Code(wblk, m_Param.m_nWTlevels, m_Param.m_nLossyBitPlanes);

				nbBlock++;
				if (m_Param.m_RestartInterval && nbBlock == m_Param.m_RestartInterval)
				{
					// Output a restart marker
					nbBlock = 0;
					ac.Stop();
					m_Buf.write_marker(c_MarkerRESTART | (markerNum & 0xF));
					markerNum++;
					if (bX < (nbW - 1) || bY < (nbH - 1))
						ac.Start();
					else
						bACStopped = true;
					// Reset VLC decoder state (models probabilities)
					coder.Reset();
				}
			}
		}

		// Stop the arithmetic coding
		if (!bACStopped)
			ac.Stop();

		// Write the Footer marker
		m_Buf.write_marker(c_MarkerFOOTER);

		COMP_CATCHTHIS
	}

	void CWTCoder::CodeBufferFull(void)
	{
		COMP_TRYTHIS
		// Write header
		m_Buf.write_marker(c_MarkerHEADER);
		m_Buf.real_write(m_Param.m_BitsPerPixel, 4);
		m_Buf.real_write(m_Image.GetW(), 16);
		m_Buf.real_write(m_Image.GetH(), 16);
		m_Buf.real_write(m_Param.m_nWTlevels - 3, 2);
		m_Buf.real_write(m_Param.m_PredMode - CWTParams::e_None, 2);
		m_Buf.real_write(3, 2); // Full block
		m_Buf.real_write(m_Param.m_RestartInterval, 16);
		m_Buf.real_write(m_Param.m_nLossyBitPlanes, 4);
		m_Buf.real_write(0, 2); // Pad bits
		m_Buf.write_marker(c_MarkerDATA);

		// Start arithmetic coder
		CACCoder ac(m_Buf);
		ac.Start();

		// Compute the Wavelet block size to allow m_nWTlevels S+P iterations
		int bW = m_Image.GetW();
		int bH = m_Image.GetH();
		const int iteS = 1U << m_Param.m_nWTlevels;
		bW = (bW + iteS - 1) & (-iteS);
		bH = (bH + iteS - 1) & (-iteS);

		// Wavelet block object creation
		CWBlock wblk(bW, bH);

		// Copy image to CWBlock with line and column padding
		wblk.GetAndPad(m_Image, 0, 0, m_Image.GetW(), m_Image.GetH());

		// Perform the forward m_nWTlevels iterations of 2D S(+P) transform
		switch (m_Param.m_PredMode)
		{
		case CWTParams::e_None:
			wblk.IterateSt(true, m_Param.m_nWTlevels);
			break;
		case CWTParams::e_PredA:
			wblk.IterateSptA(true, m_Param.m_nWTlevels);
			break;
		case CWTParams::e_PredB:
			wblk.IterateSptB(true, m_Param.m_nWTlevels);
			break;
		case CWTParams::e_PredC:
			wblk.IterateSptC(true, m_Param.m_nWTlevels);
			break;
		default:
			Assert(0, Util::CParamException())
		}

		// VLC coder object creation
		CVLCCoder coder(ac);

		// code the S+P coefs
		coder.Code(wblk, m_Param.m_nWTlevels, m_Param.m_nLossyBitPlanes);

		// Stop Arithmetic coding
		ac.Stop();

		// Write the Footer marker
		m_Buf.write_marker(c_MarkerFOOTER);

		COMP_CATCHTHIS
	}

	void CWTCoder::CodeBuffer(void)
	{
		COMP_TRYTHIS
		switch (m_Param.m_BlockMode)
		{
		case CWTParams::e_16x16Block:
			Assert(m_Param.m_nWTlevels <= 4, Util::CParamException());
			CodeBufferBlock(16U);
			break;
		case CWTParams::e_32x32Block:
			Assert(m_Param.m_nWTlevels <= 5, Util::CParamException());
			CodeBufferBlock(32U);
			break;
		case CWTParams::e_64x64Block:
			// Assert not really needed...
			Assert(m_Param.m_nWTlevels <= 6, Util::CParamException());
			CodeBufferBlock(64U);
			break;
		case CWTParams::e_FullWidth:
			CodeBufferFull();
			break;
		default:
			Assert(0, Util::CParamException())
		}
		m_Buf.close();
		COMP_CATCHTHIS
	}

} // end namespace
