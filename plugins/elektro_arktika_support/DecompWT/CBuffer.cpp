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
//	FileName:		CBuffer.cpp
//
//	Date Created:	5/3/1998
//
//	Author:			Van Wynsberghe Laurent
//
//
//	Description:	These classes provide a read-buffer and a write-buffer
//					These buffers can be accessed at bit-level,
//					so it is possible to write a bit string without byte alignment
//
///	Last Modified:	$Dmae: 1999/05/12 10:40:02 $
//
//  RCS Id:			$Id: CBuffer.cpp,v 1.40 1999/05/12 10:40:02 youag Exp $
//
//	$Rma: CBuffer.cpp,v $
//	Revision 1.40  1999/05/12 10:40:02  youag
//	Added CWBuffer::double_size() function to limit some inline functions size
//
//	Revision 1.39  1999/03/01 20:07:40  xne
//	(Re)introducing CVS/RCS commit messages
//
//  Revision 1.14  1998/07/06 21:39:40  xne
//  Made the necessary modifications to comply with the new directory structure.
//
//  Revision 1.13  1998/06/29 18:46:41  xne
//  Introduced namespace COMP.
//
//  Revision 1.12  1998/06/29 16:43:41  xne
//  Introduced error detection & recovery in lossy/lossless codec.
//
//  Revision 1.11  1998/05/26 06:48:25  lvwynsb
//  New class CBlock to manipulate generic 8*8 blocks
//  This class is really simple and is faster than the CMatrix<T> one
//
//  Revision 1.10  1998/05/19 20:06:48  lvwynsb
//  - cosmetic changes
//
//  Revision 1.8  1998/05/19 19:22:46  lvwynsb
//  - Cleaning the code, add comments
//
////////////////////////////////////////////////////////////////////////////
//:End Ignore

#include <memory>
#include <iostream>

#include "ErrorHandling.h"
#include "CBuffer.h"

using namespace std;

namespace COMP
{
	CRBuffer::CRBuffer(const Util::CDataField &i_cdf)
		: CBuffer(i_cdf)
	{
		COMP_TRYTHIS
		m_byte_index = 0;
		m_bit_buffer = 0;
		m_bit_index = 0;
		m_byte_IN = 0;
		m_marker_position = -1;
		m_marker_offset = 0;
		m_reached_end = false;
		// Load the 40 first bits : put 32 in m_bit_buffer and 8 in m_byte_IN
		get_first_byte();
		while (m_bit_index <= 32)
			get_next_byte();
		m_bit_index -= 8;
		COMP_CATCHTHIS
	}

	CRBuffer::CRBuffer(const CWBuffer &i_W)
		: CBuffer((const Util::CDataField)i_W)
	{
		COMP_TRYTHIS
		// Initialise a reading buffer from a writing buffer
		m_byte_index = 0;
		m_bit_buffer = 0;
		m_bit_index = 0;
		m_byte_IN = 0;
		m_marker_position = -1;
		m_marker_offset = 0;
		m_reached_end = false;
		// Load the 40 first bits : put 32 in m_bit_buffer and 8 in m_byte_IN
		get_first_byte();
		while (m_bit_index <= 32)
			get_next_byte();
		m_bit_index -= 8;
		COMP_CATCHTHIS
	}

	CRBuffer::~CRBuffer()
	{
	}

	// GENERAL CONTROL FUNCTIONS
	void CRBuffer::rewind(void)
	{
		COMP_TRYTHIS
		m_byte_index = 0;
		m_bit_buffer = 0;
		m_bit_index = 0;
		m_byte_IN = 0;
		m_marker_position = -1;
		m_marker_offset = 0;
		m_reached_end = false;
		// Load the 40 first bits : put 32 in m_bit_buffer and 8 in m_byte_IN
		get_first_byte();
		while (m_bit_index <= 32)
			get_next_byte();
		m_bit_index -= 8;
		COMP_CATCHTHIS
	}

	void CRBuffer::real_rewind()
	{
		COMP_TRYTHIS
		m_byte_index = 0;
		m_bit_buffer = 0;
		m_bit_index = 0;
		m_byte_IN = 0;
		m_marker_position = -1;
		m_marker_offset = 0;
		m_reached_end = false;
		// Load the 40 first bits : put 32 in m_bit_buffer and 8 in m_byte_IN
		get_first_byte();
		while (m_bit_index <= 32)
			get_real_next_byte();
		m_bit_index -= 8;
		COMP_CATCHTHIS
	}

	void CRBuffer::resync(void)
	{
		COMP_TRYTHIS
		if (m_byte_index >= 4)
		{
			m_byte_index -= 4;
			m_bit_buffer = 0;
			m_bit_index = 0;
			m_byte_IN = 0;
			m_marker_position = -1;
			m_marker_offset = 0;
			m_reached_end = false;
			// Load the 40 first bits : put 32 in m_bit_buffer and 8 in m_byte_IN
			get_first_byte();
			while (m_bit_index <= 32)
				get_next_byte();
			m_bit_index -= 8;
		}
		COMP_CATCHTHIS
	}
	// ==================
	// Buffer for WRITING
	// ==================

	// CONSTRUCTOR
	CWBuffer::CWBuffer(const unsigned __int32 &i_sizeInBytes)
		: CBuffer(Util::CDataField(i_sizeInBytes * 8, false))
	{
		COMP_TRYTHIS
		m_byte_index = (unsigned int)-1; // index of the last byte written...
		m_byte_OUT = 0;
		m_bit_index = 0;
		COMP_CATCHTHIS
	}

	// DESTRUCTOR
	CWBuffer::~CWBuffer()
	{
	}

	void CWBuffer::double_size(void)
	{
		COMP_TRYTHIS
		SetLength(m_size << 4);
		m_size = (unsigned __int32)((GetLength() + 7) >> 3);
		m_ptr = get();
		COMP_CATCHTHIS
	}

} //namespace COMP
