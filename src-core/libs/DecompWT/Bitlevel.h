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

#ifndef BitLevel_included
#define BitLevel_included

/*******************************************************************************

TYPE:
Free functions

PURPOSE:	
interface for the bit-mask functions

FUNCTION:	
These functions make the bit-masks used in the Huffman coding/decoding process
      
INTERFACES:	
See 'INTERFACES' in the module declaration below
      
RESOURCES:	
Stack memory (>2K).
      
REFERENCE:	
None

PROCESSING:	
All the possibles masks are stored into local tables for each function

DATA:
None		
      
LOGIC:
      
*******************************************************************************/

#include "ErrorHandling.h"
#include "RMAErrorHandling.h"

namespace COMP
{
	// Description:	Produce a 32 bits mask with 'length' bits set to 1 followed by 'start' (lsb) bits set to 0
	// Returns:		the produced mask
	inline unsigned __int32 speed_mask(
		const unsigned int &i_length, // number of bits set to 1
		const unsigned int &i_start	  // number of lsb bits set to 0
	)
	{
		COMP_TRYTHIS_SPEED
		// makes a bit string with 'i_length' ms 1bits followed by 'i_start' ls 0bits
		static const unsigned __int32 ref_mask[33] = {
			0x00000000,
			0x00000001, 0x00000003, 0x00000007, 0x0000000F,
			0x0000001F, 0x0000003F, 0x0000007F, 0x000000FF,
			0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF,
			0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF,
			0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF,
			0x001FFFFF, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF,
			0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF,
			0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF};
#ifdef _DEBUG
		Assert(i_length < 33 && i_start < 32, Util::CParamException());
#endif
		return (ref_mask[i_length] << i_start);
		COMP_CATCHTHIS_SPEED
	}

	// Description:	returns a '000011....11' 16 bits mask with 'length' bits set to 1
	// Returns:		the produced mask
	inline unsigned __int16 speed_mask16_lsb(
		const unsigned int &i_length // number of '1' bits
	)
	{
		COMP_TRYTHIS_SPEED
		// makes a 000011....11' 16 bits mask with 'length' 1 bits
		static const unsigned __int16 ref_mask[17] = {
			0x0000,
			0x0001, 0x0003, 0x0007, 0x000F,
			0x001F, 0x003F, 0x007F, 0x00FF,
			0x01FF, 0x03FF, 0x07FF, 0x0FFF,
			0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF};
#ifdef _DEBUG
		Assert(i_length < 17, Util::CParamException());
#endif
		return ref_mask[i_length];
		COMP_CATCHTHIS_SPEED
	}

	// Description:	returns a '11..100000' 16 bits mask with 'length' bits set to 1
	// Returns:		the produced mask
	inline unsigned __int16 speed_mask16_msb(
		const unsigned int &i_length // number of '1' bits
	)
	{
		COMP_TRYTHIS_SPEED
		// Returns a '11..100000' 16 bits mask with 'length' 1 bits
		static const unsigned __int16 ref_mask[17] = {
			0x0000,
			0x8000, 0xC000, 0xE000, 0xF000,
			0xF800, 0xFC00, 0xFE00, 0xFF00,
			0xFF80, 0xFFC0, 0xFFE0, 0xFFF0,
			0xFFF8, 0xFFFC, 0xFFFE, 0xFFFF};
#ifdef _DEBUG
		Assert(i_length < 17, Util::CParamException());
#endif
		return ref_mask[i_length];
		COMP_CATCHTHIS_SPEED
	}

	// Description:	returns a '11..100000' 32 bits mask with 'length' bits set to 1
	// Returns:		the produced mask
	inline unsigned __int32 speed_mask32_msb(
		const unsigned int &i_length // number of '1' bits
	)
	{
		COMP_TRYTHIS_SPEED
		// Returns a '11..100000' 32 bits mask with 'length' 1 bits
		static const unsigned __int32 ref_mask[33] = {
			0x00000000,
			0x80000000, 0xC0000000, 0xE0000000, 0xF0000000,
			0xF8000000, 0xFC000000, 0xFE000000, 0xFF000000,
			0xFF800000, 0xFFC00000, 0xFFE00000, 0xFFF00000,
			0xFFF80000, 0xFFFC0000, 0xFFFE0000, 0xFFFF0000,
			0xFFFF8000, 0xFFFFC000, 0xFFFFE000, 0xFFFFF000,
			0xFFFFF800, 0xFFFFFC00, 0xFFFFFE00, 0xFFFFFF00,
			0xFFFFFF80, 0xFFFFFFC0, 0xFFFFFFE0, 0xFFFFFFF0,
			0xFFFFFFF8, 0xFFFFFFFC, 0xFFFFFFFE, 0xFFFFFFFF};
#ifdef _DEBUG
		Assert(i_length < 33, Util::CParamException());
#endif
		return ref_mask[i_length];
		COMP_CATCHTHIS_SPEED
	}

	// Description:	returns a '000011....11' 32 bits mask with 'length' bits set to 1
	// Returns:		the produced mask
	inline unsigned __int32 speed_mask32_lsb(
		const unsigned int &i_length // number of '1' bits
	)
	{
		COMP_TRYTHIS_SPEED
		// Returns a '11..100000' 32 bits mask with 'length' 1 bits
		static const unsigned __int32 ref_mask[33] = {
			0x00000000,
			0x00000001, 0x00000003, 0x00000007, 0x0000000F,
			0x0000001F, 0x0000003F, 0x0000007F, 0x000000FF,
			0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF,
			0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF,
			0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF,
			0x001FFFFF, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF,
			0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF,
			0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF};
#ifdef _DEBUG
		Assert(i_length < 33, Util::CParamException());
#endif
		return ref_mask[i_length];
		COMP_CATCHTHIS_SPEED
	}

	// Description:	return a 16 bits mask with the 'bit' set to 1
	//				bit=0 is the lsb
	// Returns:		the produced mask
	inline unsigned __int16 speed_bit16(
		const unsigned int &i_bit // the bit number to set to 1
	)
	{
		COMP_TRYTHIS_SPEED
		// set the specified bit to 1
		static const unsigned __int16 ref_mask[17] = {
			0x0000,
			0x0001, 0x0002, 0x0004, 0x0008,
			0x0010, 0x0020, 0x0040, 0x0080,
			0x0100, 0x0200, 0x0400, 0x0800,
			0x1000, 0x2000, 0x4000, 0x8000};
#ifdef _DEBUG
		Assert(i_bit < 17, Util::CParamException());
#endif
		return ref_mask[i_bit];
		COMP_CATCHTHIS_SPEED
	}

	// Description:	return a 32 bits mask with the 'bit' set to 1
	//				bit=0 is the lsb
	// Returns:		the produced mask
	inline unsigned __int32 speed_bit32(
		const unsigned int &i_bit // the bit number to set to 1
	)
	{
		COMP_TRYTHIS_SPEED
		// set the specified bit to 1
		static const unsigned __int32 ref_mask[33] = {
			0x00000000,
			0x00000001, 0x00000002, 0x00000004, 0x00000008,
			0x00000010, 0x00000020, 0x00000040, 0x00000080,
			0x00000100, 0x00000200, 0x00000400, 0x00000800,
			0x00001000, 0x00002000, 0x00004000, 0x00008000,
			0x00010000, 0x00020000, 0x00040000, 0x00080000,
			0x00100000, 0x00200000, 0x00400000, 0x00800000,
			0x01000000, 0x02000000, 0x04000000, 0x08000000,
			0x10000000, 0x20000000, 0x40000000, 0x80000000};
#ifdef _DEBUG
		Assert(i_bit < 33, Util::CParamException());
#endif
		return ref_mask[i_bit];
		COMP_CATCHTHIS_SPEED
	}

	// Description:	maps a coefficient to a SSSS value
	//				SSSS is the number of bits needed to express the coef in binary
	// Returns:		SSSS
	inline unsigned int speed_csize(
		__int32 i_coeff // coefficient value
	)
	{
		COMP_TRYTHIS_SPEED
		static const unsigned int lut[1024] = {
			0,
			1,
			2, 2,
			3, 3, 3, 3,
			4, 4, 4, 4, 4, 4, 4, 4,
			5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
			6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
			7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
			7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
			9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
			9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
			9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
			9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
			9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
			9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
			9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
			9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
			10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10};

		// maps a coefficient to a SSSS value
		if (i_coeff < 0L)
			i_coeff = -i_coeff;
		if (i_coeff < 1024L)
			return lut[i_coeff];
		if (!(i_coeff >>= 11))
			return 11U;
		unsigned int n = 12U;
		while ((i_coeff >>= 1))
			n++;
		return n;
		COMP_CATCHTHIS_SPEED
	}

} // end namespace

#endif
