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

#ifndef CBuffer_included
#define CBuffer_included

/*******************************************************************************

TYPE:
CBuffer : Abstract class
CWBuffer : concrete class
CRBuffer : concrete class

PURPOSE:
These classes handles the bit buffer objects needed to store the compressed images

FUNCTION:
The CWBuffer enables to write a bit string (from 1 to 32 bits long) at the current
position in the buffer, to modify the current position, to write markers into the
bit buffer.
The CRBuffer enables to read a bit string (32 bits) from the current position,
modify the current position and to write markers.
      
INTERFACES:	
See 'INTERFACES' in the module declaration below
      
RESOURCES:	
Heap Memory (>2K).
      
REFERENCE:	
None

PROCESSING:	
CWBuffer :
	The binary strings of variable length are accepted by the "write" method.
	The string is then divided into bytes and the remaining part (incomplete byte)
	is stored into the bit buffer.  Each complete byte is writed in the buffer
	via the "put_next_byte" method. The contents of the bit buffer will be used 
	for the next "write", and will be added to the first bits of the new binary
	string to make a whole byte.

CRBuffer : 
	For each call of the method "read32", we first take the actual contents of 
	the bit buffer (m_bit_buffer) to make the beginning og the output string.
	Then we fetch this bit buffer with the following bytes via the "get_next_byte"
	method.  Then we take in this buffer as many bits as needed to get the total
	32 bits output string.

DATA:
See 'Data' in the class header below.			
      
LOGIC:
      
*******************************************************************************/

#include <memory>

#include "CDataField.h"
#include "Bitlevel.h"

namespace COMP
{

	class CWBuffer;

	class CBuffer : public Util::CDataField
	{

	protected:
		// DATA:

		unsigned __int32 m_byte_index; // R: index of m_byte_IN in m_ap_byte_buffer
									   // W: index of the last byte written to the buffer.
		unsigned __int32 m_size;	   // size of the buffer (in bytes)
		unsigned char *m_ptr;		   // pointer to te start of the buffer

		// Interface for General Control Functions

	public:
		// INTERFACES:

		// Description:	Constructor for father object CBuffer.
		// Returns:		Nothing.
		inline CBuffer(
			const Util::CDataField &i_cdf // CDF from which to construct the buffer
			)
			: Util::CDataField(i_cdf)
		{
			COMP_TRYTHIS
			m_byte_index = 0;
			m_size = (unsigned __int32)((GetLength() + 7) >> 3);
			m_ptr = get();
			COMP_CATCHTHIS
		}

		virtual void byteAlign(void) = 0;

		virtual void seek(const unsigned int i_nbBits) = 0;

		inline unsigned __int32 getByteIndex() const { return m_byte_index; }
		inline unsigned __int32 getSize() const { return m_size; }
	};

	class CRBuffer : public COMP::CBuffer
	{

	private:
		// INTERFACES:

		// Description:	Put the first byte of the buffer into m_byte_IN.
		// Returns:		Nothing.
		inline void get_first_byte()
		{
			COMP_TRYTHIS
			m_bit_buffer <<= 8;
			m_bit_buffer |= m_byte_IN;
			m_bit_index += 8;
			m_byte_IN = m_ptr[m_byte_index];
			COMP_CATCHTHIS
		}

		// Description:	Put the byte pointed by m_byte_index into m_byte_IN.
		// Returns:		Nothing.
		inline void get_next_byte()
		{
			COMP_TRYTHIS_SPEED
			const bool check_marker = m_byte_IN == 0xFF ? true : false;
			m_bit_buffer <<= 8;
			m_bit_buffer |= m_byte_IN;
			m_bit_index += 8;
			m_marker_position -= 8;
			if (m_marker_position < 0 && m_marker_offset)
			{
				m_marker_position += m_marker_offset;
				m_marker_offset = 0;
			}
			if (m_size <= ++m_byte_index)
			{
				m_byte_IN = 0;
				if ((m_size + 4) <= m_byte_index)
					m_reached_end = true;
				return;
			}
			m_byte_IN = m_ptr[m_byte_index];
			if (check_marker)
			{
				if (m_byte_IN == 0)
					// We must discard the '00' byte
					if (m_size <= ++m_byte_index)
					{
						m_byte_IN = 0;
						if ((m_size + 4) <= m_byte_index)
							m_reached_end = true;
						return;
					}
					else
						m_byte_IN = m_ptr[m_byte_index];
				else
					// We have detected a marker pattern
					if (m_marker_position >= 0)
					m_marker_offset = 24 - m_marker_position;
				else
					m_marker_position = 24;
			}
			COMP_CATCHTHIS_SPEED
		}

		// Description: Same as 'get_next_byte' but without 'FF00XX' -> 'FFXX' conversion.
		// Returns:		Nothing.
		inline void get_real_next_byte()
		{
			COMP_TRYTHIS
			const bool check_marker = m_byte_IN == 0xFF ? true : false;
			m_bit_buffer <<= 8;
			m_bit_buffer |= m_byte_IN;
			m_bit_index += 8;
			if ((m_marker_position -= 8) < 0 && m_marker_offset)
			{
				m_marker_position += m_marker_offset;
				m_marker_offset = 0;
			}
			if (m_size <= ++m_byte_index)
			{
				m_byte_IN = 0;
				if ((m_size + 4) <= m_byte_index)
					m_reached_end = true;
				return;
			}
			m_byte_IN = m_ptr[m_byte_index];
			if (check_marker)
				if (m_byte_IN != 0)
				{
					// We have detected a marker pattern
					if (m_marker_position >= 0)
						m_marker_offset = 24 - m_marker_position;
					else
						m_marker_position = 24;
				}
			COMP_CATCHTHIS
		}

	protected:
		// DATA:

		unsigned __int32 m_bit_buffer; // bit buffer
		unsigned __int8 m_byte_IN;	   // last read byte
		int m_bit_index;			   // remaining valid bits in the bit buffer
		bool m_reached_end;			   // true when the last byte was read
		int m_marker_position;		   // starting position of the FFXX pattern (position 0 is m_bit_buffer msb)
									   // negative value is used if no marker is present in m_bit_buffer
		unsigned int m_marker_offset;  // In the case two successive marker are found in a 32 bits pattern
									   // 0 if no second marker

	public:
		// INTERFACES:

		// Description:	Constructor.
		// Returns:		Nothing.
		CRBuffer(
			const Util::CDataField &i_cdf // CDF from which the buffer is build
		);

		// Description:	Constructor used to read a CWBuffer.
		// Returns:		Nothing.
		CRBuffer(
			const CWBuffer &i_W);

		// Description:	Destructor.
		// Returns:		Nothing.
		~CRBuffer();

		// Description:	Read the next 32 bits (from the current position).
		// Returns:		The output binary string.
		unsigned __int32 read32()
		{
			COMP_TRYTHIS_SPEED
			// take remaining bits from bit buffer
			// + taking the (32-m_bit_index) msb bits from m_byte_IN
			return (m_bit_buffer << (32 - m_bit_index)) | (m_byte_IN >> (m_bit_index - 24));
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Read the next n bits (from the current position).
		// Returns:		the output binary string
		unsigned __int32 readN(
			const unsigned int &n // Number of bits
		)
		{
			COMP_TRYTHIS_SPEED
#ifdef DEBUG
			Assert(n <= m_bit_index, Util::CParamException());
#endif
			// take n bits from bit buffer
			return (m_bit_buffer >> (m_bit_index - n)) & ((1UL << n) - 1UL);
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Read a marker at the current position.
		// Returns:		true if a marker was found at the current position, else returns false
		bool read_marker(
			unsigned short &o_code // the value of the marker is passed by reference
								   // the initial value is meaningless
		)
		{
			COMP_TRYTHIS
			byteAlign();
			o_code = (unsigned short)readN(16);
			// marker code must be between 0xFF01 and 0xFFFF
			return (!in_marker(1) || o_code <= 0xFF00 ? false : true);
			COMP_CATCHTHIS
		}

		// Description:	Checks whether the buffer reached the end.
		// Returns:		true if the buffer reached the end.
		inline bool reached_end() const
		{
			return m_reached_end;
		}

		// Description: Move the current position (forward only).
		// Returns:		Nothing.
		inline void seek(
			const unsigned int i_nbBits // number of bits to skip
		)
		{
			COMP_TRYTHIS_SPEED
			m_bit_index -= i_nbBits;
			// fetch the buffer
			while (m_bit_index <= 24)
				get_next_byte();
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Move the current position (forward only) and fetch the m_byteIN member
		//				without 'FF00XX' -> 'FFXX' conversion.
		// Returns:		Nothing.
		inline void real_seek(
			const unsigned int &i_nbBits // number of bits to skip
		)
		{
			COMP_TRYTHIS
			m_bit_index -= i_nbBits;
			// fetch the buffer
			while (m_bit_index <= 24)
				get_real_next_byte();
			COMP_CATCHTHIS
		}

		// Description:	Checks whether the next i_nb_bits include some marker's bits.
		// Returns:		true if the next i_nb_bits includes some marker's bits.
		inline bool in_marker(
			const unsigned int i_nb_bits // number of bits to examine
		)
		{
			COMP_TRYTHIS_SPEED
			return (m_marker_position >= 0 && i_nb_bits > (unsigned int)(m_marker_position - (32 - m_bit_index)));
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Move the current position to the beginning of the buffer.
		// Returns:		Nothing.
		void rewind();

		// Description:	Move the current position to the beginning of the buffer
		//				without 'FF00XX' -> 'FFXX' conversion.
		// Returns:		Nothing.
		void real_rewind();

		// Description:	With 'FF00XX' -> 'FFXX' conversion.
		// Returns:		Nothing.
		void resync();

		// Description:	Move the current position to the first bit of the next byte.
		// Returns:		Nothing.
		inline void byteAlign()
		{
			COMP_TRYTHIS
			seek(m_bit_index & 0x7);
			COMP_CATCHTHIS
		}
	};

	class CWBuffer : public COMP::CBuffer
	{

	private:
		// INTERFACES:

		// Description:	Writes the specified byte at the current byte position in the buffer
		//				without 'FF' -> 'FF00' conversion.
		// Returns:		Nothing.
		inline void put_real_next_byte(
			const unsigned __int8 i_byte // byte to write
		)
		{
			COMP_TRYTHIS_SPEED
			// Simply put the specified byte in the byte buffer
			if (++m_byte_index >= m_size)
				double_size();
			m_ptr[m_byte_index] = i_byte;
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Writes the specified byte at the current byte position in the buffer.
		// Returns:		Nothing.
		inline void put_next_byte(
			const unsigned __int8 i_byte // byte to write
		)
		{
			COMP_TRYTHIS_SPEED
			put_real_next_byte(i_byte);
			// Stuff one zero byte if needed
			if (i_byte == 0xFFU)
				put_real_next_byte(0x00U);
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Writes the specified byte at the current byte position in the buffer
		//				without checking buffer size.
		// Returns:		Nothing.
		inline void put_next_byte_safe(
			const unsigned __int8 i_byte // byte to write
		)
		{
			COMP_TRYTHIS_SPEED
			m_ptr[++m_byte_index] = i_byte;
			if (i_byte == 0xFFU)
				m_ptr[++m_byte_index] = 0;
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Double the size of the buffer.
		// Returns:		Nothing.
		void double_size();

		// DATA:

	protected:
		unsigned __int8 m_byte_OUT; // bit buffer
		int m_bit_index;			// actual number of valid bits in the bit buffer

		// INTERFACES:

	public:
		// Description:	Constructor.
		// Returns:		Nothing.
		CWBuffer(
			const unsigned __int32 &i_sizeInBytes // Size in bytes
		);

		// Description:	Destructor.
		// Returns:		Nothing.
		~CWBuffer();

		// Description:	Writes the binary string at the current position.
		// Returns:		Nothing.
		inline void write(
			const unsigned __int32 &i_str_bin, // binary string
			const unsigned int &i_count		   // number of valid bits (starting from the lsb)
		)
		{
			COMP_TRYTHIS_SPEED
			if ((m_bit_index + i_count) > 7)
			{
				// making a byte from s_bit_buff + part of i_str_bin
				const unsigned int n = 8U - m_bit_index;
				m_bit_index = i_count - n;
				m_byte_OUT <<= n;
				m_byte_OUT |= (i_str_bin >> m_bit_index) & ((1UL << n) - 1UL);
				put_next_byte(m_byte_OUT);
				while (m_bit_index > 7)
				{
					// put the packets of 8 bits into the byte buffer
					m_bit_index -= 8;
					m_byte_OUT = (unsigned __int8)((i_str_bin >> m_bit_index) & 0xFFUL);
					put_next_byte(m_byte_OUT);
				}
				// put the last bits of i_str_bin into the bits buffer
				m_byte_OUT = (unsigned __int8)(i_str_bin & ((1UL << m_bit_index) - 1UL));
			}
			else
			{
				// put the bits in the bits buffer
				m_bit_index += i_count;
				m_byte_OUT <<= i_count;
				m_byte_OUT |= i_str_bin & ((1UL << i_count) - 1UL);
			}
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Write a 32 bits binary string aligned to byte boundary.
		// Returns:		Nothing.
		inline void write32_aligned(
			const unsigned __int32 i_str_bin // 32 bits binary string
		)
		{
			COMP_TRYTHIS_SPEED
			/*if ((m_byte_index + 8) >= m_size) double_size ();
		put_next_byte_safe (i_str_bin >> 24);
		put_next_byte_safe (i_str_bin >> 16);
		put_next_byte_safe (i_str_bin >> 8);
		put_next_byte_safe (i_str_bin);*/
			unsigned __int32 n = m_byte_index;
			if ((n + 8) >= m_size)
				double_size();
			unsigned char *p = m_ptr;
			for (int i = 24; i >= 0; i -= 8)
			{
				const unsigned __int8 b = (unsigned __int8)(i_str_bin >> i);
				p[++n] = b;
				if (b == 0xFFU)
					p[++n] = 0x00;
			}
			m_byte_index = n;
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Writes the binary string at the current position
		//				without 'FF' -> 'FF00' conversion.
		// Returns:		Nothing.
		inline void real_write(
			const unsigned __int32 &i_str_bin, // binary string
			const unsigned int &i_count		   // number of valid bits (starting from the lsb)
		)
		{
			COMP_TRYTHIS
			if ((m_bit_index + i_count) > 7)
			{
				// making a byte from s_bit_buff + part of i_str_bin
				const unsigned int n = 8U - m_bit_index;
				m_bit_index = i_count - n;
				m_byte_OUT <<= n;
				m_byte_OUT |= (i_str_bin >> m_bit_index) & ((1UL << n) - 1UL);
				put_real_next_byte(m_byte_OUT);
				while (m_bit_index > 7)
				{
					// put the packets of 8 bits into the byte buffer
					m_bit_index -= 8;
					m_byte_OUT = (unsigned __int8)((i_str_bin >> m_bit_index) & 0xFFUL);
					put_real_next_byte(m_byte_OUT);
				}
				// put the last bits of i_str_bin into the bits buffer
				m_byte_OUT = (unsigned __int8)(i_str_bin & ((1UL << m_bit_index) - 1UL));
			}
			else
			{
				// put the bits in the bits buffer
				m_bit_index += i_count;
				m_byte_OUT <<= i_count;
				m_byte_OUT |= i_str_bin & ((1UL << i_count) - 1UL);
			}
			COMP_CATCHTHIS
		}

		// Description:	Writes a marker at the current position.
		// Returns:		Nothing.
		inline void write_marker(
			const unsigned short &i_code // marker code value
		)
		{
			COMP_TRYTHIS
			byteAlign();
			// write the two bytes without care about the FF00 problem
			put_real_next_byte(i_code >> 8);
			put_real_next_byte(i_code & 0xFF);
			COMP_CATCHTHIS
		}

		// Description:	Move the current position to the first bit of the next byte.
		// Returns:		Nothing.
		inline void byteAlign()
		{
			COMP_TRYTHIS
			if (!m_bit_index)
				return;
			// Copy the bits buffer into byte buffer
			unsigned __int8 byte_OUT = m_byte_OUT << (8 - m_bit_index);
			// + bit padding with '11..1' bits
			byte_OUT |= speed_mask16_lsb(8 - m_bit_index);
			put_next_byte(byte_OUT);
			// reset all the values to zero
			m_bit_index = 0;
			COMP_CATCHTHIS
		}

		// Description:	Move the current position (forward only)
		//				the skipped bits are set to 0.
		// Returns:		Nothing.
		inline void seek(
			const unsigned int i_nbBits // number of bits to skip
		)
		{
			COMP_TRYTHIS
			m_bit_index += i_nbBits;
			if (m_bit_index >= 8)
			{
				// Copy the bits buffer into byte buffer
				put_next_byte(m_byte_OUT << (8 - m_bit_index + i_nbBits));
				m_bit_index -= 8;
				// Fill the byte buffer with NULL bytes
				while (m_bit_index >= 8)
				{
					put_next_byte(0);
					m_bit_index -= 8;
				}
			}
			else
				m_byte_OUT <<= i_nbBits;
			COMP_CATCHTHIS
		}

		// Description:	Set the length of the buffer at the current bit index.
		// Returns:		Nothing.
		inline void close()
		{
			COMP_TRYTHIS
			byteAlign();
			// m_byte_index = index of last element -> length = index + 1
			SetLength((m_byte_index + 1) << 3);
			m_size = (unsigned __int32)((GetLength() + 7) >> 3);
			COMP_CATCHTHIS
		}

		friend CRBuffer::CRBuffer(const CWBuffer &i_W);
	};

} // end namespace

#endif
