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

#include <iostream>

#include "ErrorHandling.h"
#include "CDataField.h"

#include "RMAErrorHandling.h"
#include "T4Codes.h"
#include "T4Coder.h"

namespace COMP
{

	void CT4Coder::CodeRunLength(EColor i_isWhite, short i_count)
	{
		COMP_TRYTHIS
		do
		{
			short codeIdx;
			oneCode code;
			// first, put a markup code if i_count > 63
			if (i_count > 63)
			{
				if (i_count < 1792) // first type of marker
				{
					codeIdx = (i_count >> 6) - 1; // = i_count/64-1
					if (i_isWhite)
						code = m_AllCodes.GetMarkUpWhiteCode(codeIdx);
					else
						code = m_AllCodes.GetMarkUpBlackCode(codeIdx);
				}
				else // we will have to use the extended set of markers
				{
					codeIdx = (i_count >> 6) - 28; // (i_count - 1792 ) /64
					if (codeIdx > 12)			   // the max length of an aditional code = (1792 + 12*64 =) 2560
						codeIdx = 12;
					code = m_AllCodes.GetMarkUpAddCode(codeIdx);
				}
				i_count -= code.GetCount(); // take into account what was really coded
				m_obuf.WriteLSb(code.GetCode(), (unsigned char)code.GetLength());
			}
			// now, put a terminating code,
			codeIdx = i_count;
			if (codeIdx > 63)
				codeIdx = 63;
			if (i_isWhite)
				code = m_AllCodes.GetTermWhiteCode(codeIdx);
			else
				code = m_AllCodes.GetTermBlackCode(codeIdx);
			i_count -= code.GetCount();
			m_obuf.WriteLSb(code.GetCode(), (unsigned char)code.GetLength());
			// and if i_count still > 0, loop, but since Black and White
			// run must alternate, put a dummy 0-length run.
			if (i_count > 0)
			{
				if (i_isWhite)
					CodeRunLength(e_Black, 0);
				else
					CodeRunLength(e_White, 0);
			}
		} while (i_count > 0);
		COMP_CATCHTHIS
	}

	// Code one line of the image, including the corresponding EOL
	void CT4Coder::CodeNextLine()
	{
		COMP_TRYTHIS
		unsigned long maxRun = m_width; // so we stop counting at the end of a line
		unsigned long count = 0;
		while (maxRun > 0)
		{
			count = m_ibuf.CountNextResetRun(maxRun); // count white run
			CodeRunLength(e_White, (short)count);	  // code it
			maxRun -= count;
			if (maxRun > 0)
			{
				count = m_ibuf.CountNextSetRun(maxRun); // count black run
				CodeRunLength(e_Black, (short)count);	// code it
				maxRun -= count;
			}
		}
		CodeEOL(); // we promised to code the EOL so here it is
		COMP_CATCHTHIS
	}

	void CT4Coder::CodeBuffer()
	{
		COMP_TRYTHIS
		CodeEOL();
		unsigned __int64 idxi = 0;
		unsigned __int64 idxo = 0;
		for (short i = 0; i < m_height; i++)
		{
			try
			{
				idxo = m_obuf.GetBitIndex();
				idxi = m_ibuf.GetBitIndex();
				CodeNextLine();
			}
			catch (COutOfBufferException&)
			{
				m_obuf.SetLength(2 * idxo);
				// and get ready to redo the current line
				m_obuf.SetBitIndex((unsigned long)idxo);
				m_ibuf.SetBitIndex((unsigned long)idxi);
				i--;
			}
		}
		// now, make sure there remains enough place in the buffer for the data
		// we still have to put 60 bits in the buffer.
		idxo = m_obuf.GetBitIndex();
		m_obuf.SetLength(idxo + 70); //(5*12 + 10 (10=security margin)
		CodeEOL();					 // notice only 5 EOL since there is already on EOL from the last line
		CodeEOL();
		CodeEOL();
		CodeEOL();
		CodeEOL();
		m_compressedlength = m_obuf.GetBitIndex();
		COMP_CATCHTHIS
	}

	Util::CDataFieldCompressedImage CT4Coder::GetCompressedImage()
	{
		COMP_TRYTHIS
		Assert(m_compressedlength > 0, Util::CParamException());
		// create a new CDataField with the one of m_obuf. Notice
		// that no bytecopy of the buffer takes place and that
		// the CDataField creator is (a bit) fooled since we don't tell
		// him the real length of the buffer, but the length we want
		// him to consider.
		Util::CDataFieldCompressedImage cd(m_obuf,
										   1,
										   m_width,
										   m_height);
		m_compressedlength = 0; // invalidate buffer.
		return cd;
		COMP_CATCHTHIS
	}

} // end namespace COMP
