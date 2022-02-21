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

#define NOMINMAX

#include <iostream>

#include "CDataField.h"
#include "ErrorHandling.h"
#include "CompressT4.h"
#include "RMAErrorHandling.h"
#include "T4Codes.h"
#include "T4Decoder.h"

namespace COMP
{

	// Initialize the decoder structures. If the image size given does not
	// make sense, auto determination of the size if tried.
	CT4Decoder::CT4Decoder(const Util::CDataFieldCompressedImage &i_datafield) : m_ibuf(i_datafield),
																				 m_QualityInfo(i_datafield.GetNL())
	{
		COMP_TRYTHIS
		Assert(i_datafield.GetNB() == 1, Util::CParamException());
		m_height = i_datafield.GetNL();
		m_width = i_datafield.GetNC();
		if ((m_height <= 0) || (m_width <= 0))
		{
			DecodeBuffer(); // Autodetermine the size of the input image
			m_QualityInfo.resize(m_height);
		}
		m_opbm = std::unique_ptr<CBitBuffer>(new CBitBuffer((unsigned long)m_height * m_width));
		Assert(m_opbm.get(), Util::CCLibException());
		for (unsigned int i = 0; i < m_QualityInfo.size(); i++)
			m_QualityInfo[i] = 0;
		COMP_CATCHTHIS
	}

	// Reads the input buffer till an EOL symbol is encountered.
	void CT4Decoder::SkipToEOL()
	{
		COMP_TRYTHIS
		short Z = 0;
		bool b = false;
		do
		{
			Z = m_ibuf.GetNbZeroes();
			b = m_ibuf.ReadNextBitZ();
		} while (!((Z > 10) && b));
		COMP_CATCHTHIS
	}

	// Decode the buffer (if the m_height & m_width have sensible values)
	// otherwize, runs without filling the buffer and try to determine
	// the image size from the data in the input buffer.
	void CT4Decoder::DecodeBuffer()
	{
		COMP_TRYTHIS
		// size & position managment
		//short height = 0;
		short width = 0;
		short i = 0;
		short j = 0;
		// EOF / EOL managmend
		bool EndOfFile = false;
		short NbEOL = 0;
		short Z = 0; // nb of consecutive zeroes found
		// misc.
		bool isWhite = true; // lines always begin with a white code
		bool b = false;
		// T4 codes managment
		short code = 0;
		short length = 0;
		short count = 0;

		// do we do the thing for real?
		bool real = ((m_height > 0) && (m_width > 0));
		try
		{
			SkipToEOL(); // skip the first EOL
			while (!EndOfFile)
			{
				Z = m_ibuf.GetNbZeroes();
				b = m_ibuf.ReadNextBitZ();
				if (Z > 10) // eat eventual fill bits
				{
					if (b) // we found an EOL marker
					{
						if (j == 0) // are we at the begining of a line ?
						{
							NbEOL++;
							if (NbEOL > 5)
								EndOfFile = true;
						}
						else
						{
							NbEOL = 1;
							if ((i != 0)					  // we are not at the begining of the file
								&& ((real && (j != m_width))  // we are not at the end of a line
									|| (!real && (j < width)) // we found a line shorter than expected
									))
							{ // then, we shouldn't find an EOL.
#ifdef _DEBUG
								std::cout << "Unexpected EOL at line " << i << std::endl;
#endif
								if (real)
								{
									m_opbm->ResetNextNBit(m_width - j);
									SetLineValidity(i, -j);
								}
							}
							else // expected EOL
							{
								SetLineValidity(i, j); // here, j==m_width
							}

							i++;
							if (real && (i > m_height))
							{
#ifdef _DEBUG
								std::cout << "File too long...\n";
#endif
								EndOfFile = true;
							}
						}
						// if we are dummy-running, capture the maximum width
						// this does not allow for detecting bit errors in the file...
						if (!real && (j > width))
							width = j;
						j = 0;
						isWhite = true;
						code = 0;
						length = 0;
					}
				}
				else // we are shure it is not an EOL
				{
					code <<= 1;
					if (b)
					{
						code |= 1;
					}
					length++;
					if (length > 13) // it is certainly a non valid code
					{
#ifdef _DEBUG
						std::cout << "Error decoding line " << i << " at col " << j << std::endl;
#endif
						if (real)
						{
							m_opbm->ResetNextNBit(m_width - j);
							SetLineValidity(i, -j);
						}
						j = m_width; // to guard us agains a second COutOfBuffer if SkipToEOL fails...
						SkipToEOL();
						NbEOL++;
						code = 0;
						length = 0;
						j = 0;
						isWhite = true;
						i++;
						if (real && (i > m_height))
						{
#ifdef _DEBUG
							std::cout << "File too long...\n";
#endif
							EndOfFile = true;
						}
					}
					else if ((isWhite && (length >= 4)) || (!isWhite && (length >= 2)))
					{
						count = m_AllDecodes.GetCount(code, length, isWhite);
						if (count >= 0)
						{
							code = 0;
							length = 0;
							if (real)
							{
								if (j + count > m_width)
								{
#ifdef _DEBUG
									std::cout << "Line " << i << " too long.\n";
#endif
									m_opbm->ResetNextNBit(m_width - j);
									SetLineValidity(i, -j);
									j = m_width; // to guard us agains a second COutOfBuffer if SkipToEOL fails...
									SkipToEOL();
									NbEOL++;
									count = 1000; // to avoid triggering the color reversal if count < 64
									j = -count;	  // to cover the j+=count later.
									isWhite = true;
									i++;
									if (i >= m_height)
									{
#ifdef _DEBUG
										std::cout << "File too long...\n";
#endif
										EndOfFile = true;
									}
								}
								else
								{
									if (isWhite)
									{
										m_opbm->ResetNextNBit(count);
									}
									else
									{
										m_opbm->SetNextNBit(count);
									}
								}
							}
							j += count;
							if (count < 64)
								isWhite = !isWhite;
						}
						count = 0;
					}
				}
			}
		}
		catch (COutOfBufferException&)
		{
			// If we get here, it means (at least) we lost the 6 EOL markers
			// So fill the rest of the image with white

//		Util::LogReason("Expected",Util::CNamedException("Out of buffer exception successfully handled"));
#ifdef _DEBUG
			std::cout << "Exception at (" << i << "," << j << "). Lost eof (at least)\n";
#endif
		}
		if (real)
		{
			// this is only needed in case of OutOfBuffer exception or of prematurate
			// EOF (due to a missing EOL or a bad code in the file.
			if (i < m_height)
			{
#ifdef _DEBUG
				std::cout << "There was a problem while decoding the file\n";
#endif
				//these loops should NEVER generate an OutOfBuffer Exceptions
				m_opbm->ResetNextNBit(m_width - j);
				SetLineValidity(i, -j);
				i++;
				for (; i < m_height; i++)
				{
					m_opbm->ResetNextNBit(m_width);
					SetLineValidity(i, 0);
				}
			}
		}

		if (!real)
		{
			m_height = i;
			m_width = width;
		}
		m_ibuf.ResetBitIndex();
		COMP_CATCHTHIS
	}

	// create the CDataFieldUncompressedImage from the CBitBuffer and returns it
	Util::CDataFieldUncompressedImage CT4Decoder::GetDecompressedImage()
	{
		COMP_TRYTHIS
		Util::CDataFieldUncompressedImage dfui((Util::CDataField)(*m_opbm), 1, m_width, m_height, 1);
		return dfui;
		COMP_CATCHTHIS
	}

} // end namespace
