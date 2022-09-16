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

#include "CBuffer.h"

#include "CWBlock.h"
#include "CACDecoder.h"
#include "CVLCDecoder.h"
#include "CWTDecoder.h"

namespace COMP
{

	void CWTDecoder::ZeroBlock(unsigned short i_from_line,
							   unsigned short i_from_column,
							   unsigned short i_to_line,
							   unsigned short i_to_column,
							   unsigned short i_Bs)
	{
		COMP_TRYTHIS
		unsigned short bX = i_from_column;
		unsigned short bY = i_from_line;

		//unsigned short nBW = (m_Image.GetW() + i_Bs - 1) / i_Bs; // nb blocks per line
		//unsigned short nBH = (m_Image.GetH() + i_Bs - 1) / i_Bs; // nb blocks per column
		//unsigned long nB = (unsigned long)nBH * nBW;			 // total number of blocks

		CWBlock wblk(i_Bs, i_Bs);
		wblk.Zero();
		for (; bX <= i_to_column || bY < i_to_line;)
		{
			if (bX * i_Bs >= m_Image.GetW())
			{
				bX = 0;
				bY++;
			}
			if (bY > i_to_line) // go out of here
				break;
			//		m_Image.put_block (pixels_block, corner_column, corner_line);
			unsigned int nW = ((m_Image.GetW() - bX * i_Bs) < i_Bs) ? (m_Image.GetW() - bX * i_Bs) : i_Bs;
			unsigned int nH = ((m_Image.GetH() - bY * i_Bs) < i_Bs) ? (m_Image.GetH() - bY * i_Bs) : i_Bs;
			wblk.Put(m_Image, bX * i_Bs, bY * i_Bs, nW, nH);

			bX++;
		}
		COMP_CATCHTHIS
	}
	//-----------------------------------------------------------------------
	short CWTDecoder::FindNextMarker(void)
	{
		COMP_TRYTHIS
		// resynchronization with the next marker (or at least with the EOF)
		unsigned short code;

		m_Buf.byteAlign();
		for (;;)
			if (m_Buf.read_marker(code))
			{
				if (code >= c_MarkerRESTART && code <= (c_MarkerRESTART + 15))
					return (code & 0x000F);
				if (code == c_MarkerFOOTER)
					return -2; // must return a negative value. -2 is nice for debugging.
				m_Buf.seek(8);
			}
			else
			{
				if (m_Buf.reached_end())
					return -1;
				else
					m_Buf.seek(8);
			}
		COMP_CATCHTHIS
	}
	//-----------------------------------------------------------------------
	bool CWTDecoder::PerformResync(unsigned int i_Bs,
								   unsigned int &t_markerNum,
								   unsigned int &t_nbBlock,
								   unsigned int &t_bX,
								   unsigned int &t_bY)
	{
		COMP_TRYTHIS
		// Initialize things...
		//---Just to avoid passing these parameters...
		unsigned short nBW = (m_Image.GetW() + i_Bs - 1) / i_Bs; // nb blocks per line
		unsigned short nBH = (m_Image.GetH() + i_Bs - 1) / i_Bs; // nb blocks per column
		unsigned long nB = (unsigned long)nBH * nBW;			 // total number of blocks

		unsigned int begin_bY = 0;
		unsigned int new_bX = 0;
		unsigned int new_bY = 0;
		// Find the next restart marker
		short new_RSTm = FindNextMarker();

		// Found new marker
		begin_bY = (t_markerNum * m_Param.m_RestartInterval) / nBW;
		if (new_RSTm < 0) // no usable marker found
		{
			new_bX = nBW;
			new_bY = nBH - 1;
			if (m_Param.m_RestartInterval > 0)
			{
				t_nbBlock = nB % m_Param.m_RestartInterval;
				t_markerNum = nB / m_Param.m_RestartInterval;
			}
			else
			{
				// not needed since already null
				//t_nbBlock	= 0;
				//t_markerNum	= 0;
			}
		}
		else
		{
			// a valid restart marker was found
			new_RSTm = new_RSTm - (t_markerNum % 16);
			// if new_RSTm == 0  --> assume we didn't miss 16 markers but that we found the marker we missed before
			t_markerNum += new_RSTm;
			unsigned long new_nB = (t_markerNum + 1) * m_Param.m_RestartInterval;
			if (new_nB > nB)
				new_nB = nB;
			t_nbBlock = m_Param.m_RestartInterval;
			new_bX = (new_nB - 1) % nBW;
			new_bY = (new_nB - 1) / nBW;
		}
		ZeroBlock(t_bY, t_bX, new_bY, new_bX, i_Bs);
		m_Qinfo.Negate(begin_bY * i_Bs, cmin(t_bY * i_Bs + i_Bs, m_Image.GetH()) - 1);
		if (t_bY < new_bY)
			m_Qinfo.Zero(cmin(t_bY * i_Bs + i_Bs, m_Image.GetH()),
						 cmin(new_bY * i_Bs + i_Bs, m_Image.GetH()));

		// modify the returned data
		t_bX = new_bX;
		t_bY = new_bY;

		m_LastGoodLine = cmin(new_bY * i_Bs + i_Bs, m_Image.GetH());

		if (new_RSTm < 0)
			return false; // do not eat the marker, it is the EOI marker
		else
			return true;
		COMP_CATCHTHIS
	}
	//-----------------------------------------------------------------------
	bool CWTDecoder::DecodeBufferBlock(const unsigned int i_Bs)
	{
		COMP_TRYTHIS
		unsigned int bX, bY;

		// Compute the number of blocks along each dimension
		unsigned int nbW = m_Image.GetW() / i_Bs;
		const unsigned int rW = m_Image.GetW() % i_Bs;
		if (rW)
			nbW++;
		unsigned int nbH = m_Image.GetH() / i_Bs;
		const unsigned int rH = m_Image.GetH() % i_Bs;
		if (rH)
			nbH++;

		// Start the arithmetic decoder
		CACDecoder ad(m_Buf);
		ad.Start();

		// VLC decoder object creation
		CVLCDecoder decoder(ad);

		// Wavelet block object creation
		CWBlock wblk(i_Bs, i_Bs);

		// block loops
		unsigned int nbBlock = 0;	// relative block index
		unsigned int markerNum = 0; // absolute block index
		bool goodline = true;		// true if current line is ok from the beginning
		bool bACStopped = false;
		for (bY = 0; bY < nbH; bY++)
		{
			goodline = true;
			const unsigned int nH = bY == (nbH - 1) && rH ? rH : i_Bs;
			for (bX = 0; bX < nbW; bX++)
			{
				const unsigned int nW = bX == (nbW - 1) && rW ? rW : i_Bs;

				// Decode the stream
				if (!decoder.Decode(wblk, m_Param.m_nWTlevels, m_Param.m_nLossyBitPlanes) || ad.IsMarkerReached())
				{
					// Decode failed and/or a marker was reached during decoding -> error
					if (goodline)
					{
						// Set quality info
						m_LastGoodLine = bY * i_Bs + nH;
						m_Qinfo.Set(bY * i_Bs, m_LastGoodLine - 1, (-1) * (int)bX * (int)i_Bs);
					}
					goodline = false;
					PerformResync(i_Bs, markerNum, nbBlock, bX, bY);
				}
				else
				{
					switch (m_Param.m_PredMode)
					{
					case CWTParams::e_None:
						wblk.IterateSt(false, m_Param.m_nWTlevels);
						break;
					case CWTParams::e_PredA:
						wblk.IterateSptA(false, m_Param.m_nWTlevels);
						break;
					case CWTParams::e_PredB:
						wblk.IterateSptB(false, m_Param.m_nWTlevels);
						break;
					case CWTParams::e_PredC:
						wblk.IterateSptC(false, m_Param.m_nWTlevels);
						break;
					default:
						Assert(0, Util::CParamException())
					}
					// Copy CWBlock to image
					wblk.Put(m_Image, bX * i_Bs, bY * i_Bs, nW, nH);
					nbBlock++;
				}

				if (m_Param.m_RestartInterval && nbBlock == m_Param.m_RestartInterval)
				{
					unsigned short marker;

					// Check if the Restart marker is there
					nbBlock = 0;
					ad.Stop();
					if (!m_Buf.read_marker(marker) || marker != (c_MarkerRESTART | (markerNum & 0xF)))
					{
						// Restart marker not found

						// Perform resynchronization
						if (goodline)
						{
							// Set quality info
							m_LastGoodLine = bY * i_Bs + nH;
							m_Qinfo.Set(bY * i_Bs, m_LastGoodLine - 1, (-1) * (int)(bX * i_Bs + nW));
						}
						goodline = false;
						bX++; // the current block is complete and shouldn't be reset
						if (PerformResync(i_Bs, markerNum, nbBlock, bX, bY))
							m_Buf.seek(16);
						nbBlock = 0;
					}
					else
					{
						// Restart marker found

						m_Buf.seek(16);
					}
					markerNum++;
					if (bX < (nbW - 1) || bY < (nbH - 1))
						ad.Start();
					else
						bACStopped = true;
					// Reset VLC decoder state (models probabilities)
					decoder.Reset();
				}
			}
			if (goodline)
			{
				// Set quality info
				m_LastGoodLine = bY * i_Bs + nH;
				m_Qinfo.Set(bY * i_Bs, m_LastGoodLine - 1, m_Image.GetW());
			}
			goodline = true;
		}

		// Stop the arithmetic decoder
		if (!bACStopped)
			ad.Stop();

		if (m_LastGoodLine < m_Image.GetH())
			DecodeBufferError(m_LastGoodLine, m_Image.GetH());
		else
		{
			unsigned short marker = 0;
			// Check if Footer marker is there
			if (!m_Buf.read_marker(marker) || marker != c_MarkerFOOTER)
			{

				// FOOTER marker not found

				unsigned short nBW = (m_Image.GetW() + i_Bs - 1) / i_Bs; // nb blocks per line
				unsigned short begin_bY = (markerNum * m_Param.m_RestartInterval) / nBW;

				m_Qinfo.Negate(begin_bY * i_Bs,
							   m_Image.GetH() - 1);
			}
			m_Buf.seek(16);
		}

		return true;
		COMP_CATCHTHIS
	}

	bool CWTDecoder::DecodeBufferFull(void)
	{
		COMP_TRYTHIS
		// Start the arithmetic decoder
		CACDecoder ad(m_Buf);
		ad.Start();

		// Compute CWBlock size to allow m_Param.m_nWTlevels S+P iterations
		int bW = m_Image.GetW();
		int bH = m_Image.GetH();
		const int iteS = 1U << m_Param.m_nWTlevels;
		bW = (bW + iteS - 1) & (-iteS);
		bH = (bH + iteS - 1) & (-iteS);

		// Wavelet block object creation
		CWBlock wblk(bW, bH);

		// VLC decoder object creation
		CVLCDecoder decoder(ad);

		// Decode stream
		if (!decoder.Decode(wblk, m_Param.m_nWTlevels, m_Param.m_nLossyBitPlanes) || ad.IsMarkerReached())
			// Decode failed and/or a marker was reached during decoding -> error
			return false;

		// Perform the inverse m_nWTlevels iterations of 2D S(+P) transform
		switch (m_Param.m_PredMode)
		{
		case CWTParams::e_None:
			wblk.IterateSt(false, m_Param.m_nWTlevels);
			break;
		case CWTParams::e_PredA:
			wblk.IterateSptA(false, m_Param.m_nWTlevels);
			break;
		case CWTParams::e_PredB:
			wblk.IterateSptB(false, m_Param.m_nWTlevels);
			break;
		case CWTParams::e_PredC:
			wblk.IterateSptC(false, m_Param.m_nWTlevels);
			break;
		default:
			Assert(0, Util::CParamException())
		}

		// Copy the Wavelet block coefs to image
		wblk.Put(m_Image, 0, 0, m_Image.GetW(), m_Image.GetH());

		// Stop the arithmetic decoder
		ad.Stop();

		// Set quality info
		m_Qinfo.Set(0, m_Image.GetH() - 1, m_Image.GetW());
		m_LastGoodLine = m_Image.GetH();

		unsigned short marker = 0;
		// Check if Footer marker is there (there is no restart marker in full-image mode)
		if (!m_Buf.read_marker(marker) || marker != c_MarkerFOOTER)
		{

			// FOOTER marker not found

			m_Qinfo.Negate(0, m_Image.GetH() - 1);
		}
		m_Buf.seek(16);

		return true;
		COMP_CATCHTHIS
	}

	void CWTDecoder::DecodeBuffer(void)
	{
		COMP_TRYTHIS
		unsigned short marker;

		// reset the reading function
		m_Buf.real_rewind();

		// Check if the Header marker is there
		if (!m_Buf.read_marker(marker) || marker != c_MarkerHEADER)
		{

			// HEADER marker not found

			DecodeBufferError(0, m_Image.GetH());
			return;
		}
		m_Buf.real_seek(16);

		// Original image number of bits per pixel
		m_Param.m_BitsPerPixel = m_Buf.readN(4);
		m_Buf.real_seek(4);
		if (m_Param.m_BitsPerPixel == 0)
			m_Param.m_BitsPerPixel = 16;

		// Image width
		const unsigned int imgW = m_Buf.readN(16);
		m_Buf.real_seek(16);
		if (imgW != m_Image.GetW())
		{

			// Bad image width in header

			DecodeBufferError(0, m_Image.GetH());
			return;
		}

		// Image height
		const unsigned int imgH = m_Buf.readN(16);
		m_Buf.real_seek(16);
		if (imgH != m_Image.GetH())
		{

			// Bad image height in header

			DecodeBufferError(0, m_Image.GetH());
			return;
		}

		// Number of S+P transform iterations
		m_Param.m_nWTlevels = m_Buf.readN(2) + 3;
		m_Buf.real_seek(2);

		// S+P prediction type
		m_Param.m_PredMode = (CWTParams::EWTPredictionMode)(CWTParams::e_None + m_Buf.readN(2));
		m_Buf.real_seek(2);

		// Block coding type
		const unsigned int blockType = m_Buf.readN(2);
		m_Buf.real_seek(2);

		// Restart marker interval in number of blocks
		m_Param.m_RestartInterval = m_Buf.readN(16);
		m_Buf.real_seek(16);

		// Lossy coding parameter
		m_Param.m_nLossyBitPlanes = m_Buf.readN(4);
		m_Buf.real_seek(4);

		// Skip 2 pad bits to be sure next marker dont need align
		m_Buf.real_seek(2);

		// Check if the Data marker is there
		if (!m_Buf.read_marker(marker) || marker != c_MarkerDATA)
		{

			// DATA marker not found

			DecodeBufferError(0, m_Image.GetH());
			return;
		}
		m_Buf.real_seek(16);

		m_Buf.resync();

		m_LastGoodLine = 0;
		switch (blockType)
		{
		case 0:
			m_Param.m_BlockMode = CWTParams::e_16x16Block;
			if (!DecodeBufferBlock(16U))
				std::cerr << "Can not decode blocky (16x16) buffer" << std::endl;
			break;
		case 1:
			m_Param.m_BlockMode = CWTParams::e_32x32Block;
			if (!DecodeBufferBlock(32U))
				std::cerr << "Can not decode blocky (32x32) buffer" << std::endl;
			break;
		case 2:
			m_Param.m_BlockMode = CWTParams::e_64x64Block;
			if (!DecodeBufferBlock(64U))
				std::cerr << "Can not decode blocky (64x64) buffer" << std::endl;
			break;
		case 3:
			m_Param.m_BlockMode = CWTParams::e_FullWidth;
			if (!DecodeBufferFull())
				std::cerr << "Can not decode full buffer" << std::endl;
			break;
		}

		COMP_CATCHTHIS
	}

} // end namespace
