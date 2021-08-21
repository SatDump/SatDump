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

#ifndef CBitBuffer_included
#define CBitBuffer_included

/*******************************************************************************

TYPE:
Concrete class.
      
PURPOSE:
Provide a bit-oriented buffer, tuned for binary image & T.4 coded stream
manipulations.
      
FUNCTION:
class CBitBuffer:
Holds the buffer and provides the needed access member functions.
      
INTERFACES:
See 'INTERFACES' in the module declaration below
      
RESOURCES:	
Heap Memory (>2K).
      
REFERENCE:	
none

PROCESSING:	
class CBitBuffer:
	The objects mainly consists in a buffer and a index. The index is used to track
	sequential operations (read, set,reset) on the buffer. As support to decoding,
	a counter of consecutive zero-bit is also provided.
	Constructors are provided to build a CBitBuffer from either
	  - a CDataField object
	  - scratch (i.e. the buffers is allocated during construction)
	  - an auto_ptr
	ReadNextBit(), SetNextBit(), ResetNextBit() respectively read, set and reset (=0) 
	the bit at the current position and increment the current position. If
	accessing a position out of the buffer is attempted, and COMP::COutOfBuffer
	exception is thrown.
	ReadNextBitZ() same as ReadNextBit() but maintains a counter of the number of 
	consecutive zeroes encountered and that can be queried using GetNbZeroes().
	SetNextNBit() and ResetNextNBit() respectively set and reset the next given
	number of bits.
	GetBit(), SetBit() and ResetBit() respectively read, set and reset the bit
	at the given position.
	WriteLSb() write the given number of bits of the given short to the buffer,
	incrementing the index position.
	SetBitIndex() and ResetBitIndex() respectively set to a given index and reset
	the index position.
	CountNextSetRun() and CountNextResetRun() count the next number of consecutive
	bits being respectively set (=1) or reset (=0).

DATA:
See 'DATA' in the class header below.			
      
LOGIC:
-

*******************************************************************************/

#include <string>

#include "CDataField.h"
#include "SmartPtr.h"
#include "ErrorHandling.h"

#include "RMAErrorHandling.h"

namespace COMP
{

	class CBitBuffer : public Util::CDataField
	{

	private:
		// DATA:

		unsigned long m_bufOffset; // offset (in bit) in the buffer
		unsigned long m_bitIndex;  // position (in bit) in the buffer -- from 0
		short m_NbZeroes;		   // counter of consecutive zero bits (for EOL detection)

		// INTERFACES:

		// Description:	Returns the buffer size in Bits.
		// Returns:		The buffer size in bits.
		unsigned long sizeMaxBit()
		{
			COMP_TRYTHIS
			return (unsigned long)GetLength();
			COMP_CATCHTHIS
		}

	public:
		// Description:	Constructor. Creates a BitBuffer from a CDataField.
		// Returns:		Nothing.
		CBitBuffer(
			const Util::CDataField &i_datafield, // buffer from which to construct the BitBuffer
			unsigned __int64 i_bufOffset = 0	 // start offset in the given buffer
			)
			: Util::CDataField(i_datafield), m_bufOffset((unsigned long)i_bufOffset), m_bitIndex((unsigned long)i_bufOffset) // we start reading at the index...
			  ,
			  m_NbZeroes(0)
		{
		}

		// Description:	Constructor. Creates a BitBuffer from scratch.
		// Returns:		Nothing.
		CBitBuffer(
			unsigned __int64 i_bufLength,	 // length of the buffer to create
			unsigned __int64 i_bufOffset = 0 // start offset in the given buffer
			)
			: Util::CDataField(i_bufLength, false), m_bufOffset((unsigned long)i_bufOffset), m_bitIndex(0), m_NbZeroes(0)
		{
		}

		// Description:	Constructor. Creates a BitBuffer from a SmartPtr.
		// Returns:		Nothing.
		CBitBuffer(
			Util::CSmartPtr<unsigned char> i_buf, // points to the char* buffer
			unsigned __int64 i_bufLength,		  // length of the buffer pointed to by i_buf
			unsigned __int64 i_bufOffset = 0	  // start offset in the given buffer
			)
			: Util::CDataField(i_buf.Release(), i_bufLength), m_bufOffset((unsigned char)i_bufOffset), m_bitIndex((unsigned char)i_bufOffset) // we start reading at the index...
			  ,
			  m_NbZeroes(0)
		{
		}

		// Description:	Destructor.
		// Returns:		Nothing.
		~CBitBuffer()
		{
		}

		// Description:	Reads the bit at the current index position and increment
		//				the index position.
		// Returns:		true if the bit is set (=1), false otherwise.
		bool ReadNextBit()
		{
			COMP_TRYTHIS
			Assert(m_bitIndex < sizeMaxBit(), COMP::COutOfBufferException());
			unsigned long byteIndex = m_bitIndex / 8;
			unsigned char mask = 0x80 >> (m_bitIndex & 0x7);
			m_bitIndex++;
			return ((get()[byteIndex] & mask) != 0);
			COMP_CATCHTHIS
		}

		// Description:	Reads the bit at the current index position and increment
		//				the index position, keeping a count of the number of consecutive
		//				zeroes encountered.
		// Returns:		true if the bit is set (=1), false otherwise.
		bool ReadNextBitZ()
		{
			COMP_TRYTHIS
			Assert(!(m_bitIndex >= sizeMaxBit()), COutOfBufferException());
			unsigned long byteIndex = m_bitIndex / 8;
			unsigned char mask = 0x80 >> (m_bitIndex & 0x7);
			m_bitIndex++;
			if ((get()[byteIndex] & mask) != 0)
			{
				m_NbZeroes = 0; // well, we found a bit==1
				return true;
			}
			m_NbZeroes++; // one more bit==0
			return false;
			COMP_CATCHTHIS
		}

		// Description:	Set (=1) the bit at the current index position and increment
		//				the index position.
		// Returns:		Nothing.
		void SetNextBit()
		{
			COMP_TRYTHIS
			Assert(m_bitIndex < sizeMaxBit(), COMP::COutOfBufferException());
			unsigned long byteIndex = m_bitIndex / 8;
			unsigned char mask = 1 << (7 - (m_bitIndex % 8));
			m_bitIndex++;
			get()[byteIndex] |= mask;
			COMP_CATCHTHIS
		}

		// Description:	Reset (=0) the bit at the current index position and increment
		//				the index position.
		// Returns:		Nothing.
		void ResetNextBit()
		{
			COMP_TRYTHIS
			Assert(m_bitIndex < sizeMaxBit(), COMP::COutOfBufferException());
			unsigned long byteIndex = m_bitIndex / 8;
			unsigned char mask = 1 << (7 - (m_bitIndex % 8));
			m_bitIndex++;
			get()[byteIndex] &= ~mask;
			COMP_CATCHTHIS
		}

		// Description:	Set (=1) the next i_N bits starting at the current index position
		//				and increment the index position by i_N.
		// Returns:		Nothing.
		void SetNextNBit(
			unsigned __int64 i_N // number of consecutive bits to set.
		);

		// Description:	Reset (=0) the next i_N bits starting at the current index position
		//				and increment the index position by i_N.
		// Returns:		Nothing.
		void ResetNextNBit(
			unsigned __int64 i_N // number of consecutive bits to reset.
		);

		// Description:	Set (=1) the bit at the specified index position.
		// Returns:		Nothing.
		void SetBit(
			unsigned __int64 i_idx // index position of the bit to set
		)
		{
			COMP_TRYTHIS
			Assert(i_idx < sizeMaxBit(), COMP::COutOfBufferException());
			unsigned __int64 byteIndex = i_idx / 8;
			unsigned char mask = (unsigned char)(1 << (7 - (i_idx % 8)));
			get()[byteIndex] |= mask;
			COMP_CATCHTHIS
		}

		// Description:	Reset (=0) the bit at the specified index position.
		// Returns:		Nothing.
		void ResetBit(
			unsigned __int64 i_idx // index position of the bit to reset
		)
		{
			COMP_TRYTHIS
			Assert(i_idx < sizeMaxBit(), COMP::COutOfBufferException());
			unsigned __int64 byteIndex = i_idx / 8;
			unsigned char mask = (unsigned char)(1 << (7 - (i_idx % 8)));
			get()[byteIndex] &= ~mask;
			COMP_CATCHTHIS
		}

		// Description:	Returns true if the bit at the specified index position if set,
		//				false otherwise.
		// Returns:		true if the bit is set, false otherwise.
		bool GetBit(
			unsigned __int64 i_idx // index position of the bit to query
		)
		{
			COMP_TRYTHIS
			Assert(i_idx < sizeMaxBit(), COMP::COutOfBufferException());
			unsigned __int64 byteIndex = i_idx / 8;
			unsigned char mask = (unsigned char)(1 << (7 - (i_idx % 8)));
			return ((get()[byteIndex] & mask) != 0);
			COMP_CATCHTHIS
		}

		// Description:	Returns the number of zeroes counted by the ReadNextBitZ()
		//				member function.
		// Returns:		The number of zeroes counted.
		short GetNbZeroes()
		{
			return m_NbZeroes;
		}

		// Description:	Writes the number of specified Least Significant bits,
		//				from i_data in the buffer, at the current position.
		//				Increments the buffer index by the number of bits written.
		// Returns:		Nothing.
		void WriteLSb(
			unsigned short i_data, // short containing the data to write
			unsigned char i_length // number of bits to write in the buffer.
		);

		// Description:	Returns the current bit index (index at which the next
		//				operation will happen).
		// Returns:		The current bit index.
		unsigned long GetBitIndex()
		{
			return m_bitIndex;
		}

		// Description:	Positions the index at the specified value
		// Returns:		Nothing.
		void SetBitIndex(
			unsigned long i_idx // new index position
		)
		{
			COMP_TRYTHIS
			Assert(i_idx < sizeMaxBit(), COMP::COutOfBufferException());
			m_bitIndex = i_idx;
			COMP_CATCHTHIS
		}

		// Description:	Returns the index position at its initial position (=offset)
		// Returns:		Nothing.
		void ResetBitIndex()
		{
			COMP_TRYTHIS
			m_bitIndex = m_bufOffset;
			m_NbZeroes = 0;
			COMP_CATCHTHIS
		}

		// Description:	Returns the number of consecutive bits set in the buffer, starting
		//				at the current position, but does not count more than i_max bits.
		// Returns:		The number of consecutive bits that are set
		unsigned long CountNextSetRun(
			unsigned long i_maxRun // the maximum number of bits to examine.
		);

		// Description:	Returns the number of consecutive bits reset in the buffer, starting
		//				at the current position, but does not count more than i_max bits.
		// Returns:		The number of consecutive bits that are reset.
		unsigned long CountNextResetRun(
			unsigned long i_maxRun // the maximum number of bits to examine.
		);
	};

} // end namespace

#endif
