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

#include "RMAErrorHandling.h"
#include "CBitBuffer.h" // COMP\T4

void COMP::CBitBuffer::SetNextNBit(
	unsigned __int64 i_N)
{
	COMP_TRYTHIS
	Assert(m_bitIndex + i_N > sizeMaxBit(), COutOfBufferException());

	unsigned long byteIndex = m_bitIndex >> 3; // bit position in byte

	// first, try to fill the remaining bits of the byte
	unsigned char filledbits = (unsigned char)(m_bitIndex & 7); // bitindex % 8 = bits already used
	unsigned char bitstofill = 8 - filledbits;

	if (i_N > bitstofill)
	{
		// fill the remaining bits of the byte
		unsigned char mask = (0xFF >> filledbits);
		get()[byteIndex] |= mask; //set the remaining bits
		i_N -= bitstofill;
		m_bitIndex += bitstofill;
		// now fill by entire bytes
		while (i_N >= 8)
		{
			byteIndex++;
			get()[byteIndex] = 0xFF; // all bits = 1
			i_N -= 8;
			m_bitIndex += 8;
		}
		byteIndex++;
		// and terminate the fill;
		unsigned char negmask = (unsigned char)(0xFF >> i_N);
		get()[byteIndex] |= ~(negmask);
		m_bitIndex += (unsigned long)i_N;
	}
	else
	{
		// since i_N < bitstofill, revert to looping
		for (; i_N > 0; i_N--)
			SetNextBit();
	}
	COMP_CATCHTHIS
}

void COMP::CBitBuffer::ResetNextNBit(
	unsigned __int64 i_N)
{
	COMP_TRYTHIS
	Assert(m_bitIndex + i_N <= sizeMaxBit(), COMP::COutOfBufferException());

	unsigned long byteIndex = m_bitIndex >> 3; // bit position in byte

	// first, try to fill the remaining bits of the byte
	unsigned char filledbits = (unsigned char)(m_bitIndex & 7); // bitindex % 8 = bits already used
	unsigned char bitstofill = (unsigned char)(8 - filledbits);

	if (i_N > bitstofill)
	{
		// fill the remaining bits of the byte
		unsigned char mask = (0xFF >> filledbits);
		get()[byteIndex] &= ~mask; // reset the bits
		i_N -= bitstofill;
		m_bitIndex += bitstofill;
		// fill by entire bytes
		while (i_N >= 8)
		{
			byteIndex++;
			get()[byteIndex] = 0; // all bits = 0
			i_N -= 8;
			m_bitIndex += 8;
		}
		byteIndex++;
		// now, terminate the fill;
		unsigned char negmask = (unsigned char)(0xFF >> i_N);
		get()[byteIndex] &= (negmask);
		m_bitIndex += (unsigned long)i_N;
	}
	else
	{
		// since i_N < bitstofill, revert to looping
		for (; i_N > 0; i_N--)
			ResetNextBit();
	}
	COMP_CATCHTHIS
}

void COMP::CBitBuffer::WriteLSb(
	unsigned short i_data,
	unsigned char i_length)
{
	COMP_TRYTHIS
	Assert(m_bitIndex + i_length < sizeMaxBit(), COMP::COutOfBufferException());
	//unsigned long byteIndex = m_bitIndex / 8;

	unsigned short mask = 1 << (i_length - 1);
	while (mask != 0)
	{
		if (i_data & mask)
			SetNextBit();
		else
			ResetNextBit();
		mask >>= 1;
	}
	COMP_CATCHTHIS
}

unsigned long COMP::CBitBuffer::CountNextSetRun(
	unsigned long i_maxRun)
{
	COMP_TRYTHIS
	Assert(i_maxRun > 0, Util::CParamException());
	// not sure this if is needed...
	Assert(m_bitIndex + i_maxRun <= sizeMaxBit(), COMP::COutOfBufferException());

	unsigned long count = 0;				   // Run length
	unsigned long byteIndex = m_bitIndex >> 3; // bit position in byte

	unsigned char filledbits = (unsigned char)(m_bitIndex & 7); // bitindex % 8 = bits already used
	unsigned char bitstofill = 8 - filledbits;
	unsigned char mask = (0xFF >> filledbits); // mask of the bits to consider
	unsigned char bits = get()[byteIndex];

	// Are we allowed to read that many bits && do all those bits == 1 ?
	if ((bitstofill <= i_maxRun) && ((bits & mask) == mask))
	{
		count += bitstofill;
		m_bitIndex += bitstofill;
		byteIndex++;
		i_maxRun -= bitstofill;

		// now, read by entire bytes as long as we find entire bytes
		// == 1
		while ((i_maxRun >= 8) && (get()[byteIndex] == 0xFF))
		{
			count += 8;
			m_bitIndex += 8;
			byteIndex++;
			i_maxRun -= 8;
		}
		// get ready for manual finish
		filledbits = 0;
		bits = get()[byteIndex];
	}
	// finish by hand
	mask = (0x80 >> filledbits);
	while ((bits & mask) && (i_maxRun > 0))
	{
		count++;
		m_bitIndex++;
		i_maxRun--;
		mask >>= 1;
	}
	return count;
	COMP_CATCHTHIS
}

unsigned long COMP::CBitBuffer::CountNextResetRun(
	unsigned long i_maxRun)
{
	COMP_TRYTHIS
	Assert(i_maxRun > 0, Util::CParamException());
	// not sure this if is needed...
	Assert(m_bitIndex + i_maxRun <= sizeMaxBit(), COMP::COutOfBufferException());

	unsigned long count = 0;				   // Run length
	unsigned long byteIndex = m_bitIndex >> 3; // bit position in byte

	unsigned char filledbits = (unsigned char)(m_bitIndex & 7); // bitindex % 8 = bits already used
	unsigned char bitstofill = 8 - filledbits;
	unsigned char mask = (0xFF >> filledbits);
	unsigned char bits = get()[byteIndex];

	if ((bitstofill <= i_maxRun) && !(bits & mask))
	{
		count += bitstofill;
		m_bitIndex += bitstofill;
		byteIndex++;
		i_maxRun -= bitstofill;

		while ((i_maxRun >= 8) && (get()[byteIndex] == 0x00))
		{
			count += 8;
			m_bitIndex += 8;
			byteIndex++;
			i_maxRun -= 8;
		}
		filledbits = 0;
		bits = get()[byteIndex];
	}
	// finish by hand
	mask = (0x80 >> filledbits);
	while (!(bits & mask) && (i_maxRun > 0))
	{
		count++;
		m_bitIndex++;
		i_maxRun--;
		mask >>= 1;
	}
	return count;
	COMP_CATCHTHIS
}
