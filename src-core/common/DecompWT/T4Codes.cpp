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
/////////////////////////////////////////////////////////////
//
//	FileName:		T4Codes.cpp
//
//	Date Created:	19/3/1998
//
//	Author:			Xavier Neyt
//
//
//	Description:	CCITT T4/G3 codes classes.
//
//	Last Modified:	$Dmae: 1998/11/24 10:27:30 $
//
//  RCS ID:			$Id: T4Codes.cpp,v 1.15 1998/11/24 10:27:30 xne Exp $
//
//  Revision 1.8  1998/07/07 11:34:53  xne
//  Updated include paths.
//
//  Revision 1.7  1998/07/06 20:13:41  xne
//  Made the necessary changes to comply with the new directory structure.
//
//  Revision 1.6  1998/06/27 17:09:43  xne
//  Modified namespace names MSI->DISE and RMA->COMP
//
//  Revision 1.5  1998/06/19 17:26:08  xne
//  inlined most member functions
//
//  Revision 1.4  1998/04/14 21:59:14  xne
//  Enabled namespace COMP
//  Removed all global "using namespace XXX"
//  Introduced new (14/4/98) version of the CDataField classes
//  Introduced new (14/4/98) version of Compress.h
//  Added some comments
//
//  Revision 1.3  1998/04/09 20:24:46  xne
//  Cosmetic changes
//
//////////////////////////////////////////////////////////////
//:End Ignore

#include <iostream>

#include "ErrorHandling.h"
#include "RMAErrorHandling.h"

#include "T4Codes.h"

namespace COMP
{
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	void CT4Decodes::FillWhiteHashTable(oneCode *i_XxxWhiteCodes, short i_nElem)
	{
		COMP_TRYTHIS
		short i = 0;
		short idx = 0;
		for (i = 0; i < i_nElem; i++)
		{
			idx = GetWhiteIndex(i_XxxWhiteCodes[i].GetCode(),
								i_XxxWhiteCodes[i].GetLength());
			//#ifdef _DEBUG  // closed hash table->entry should be empty
			Assert(m_WhiteHashTable[idx].GetCode() == -1, Util::CParamException());
			//#endif
			m_WhiteHashTable[idx] = i_XxxWhiteCodes[i];
		}
		COMP_CATCHTHIS
	}
	//-----------------------------------------------------------------------
	void CT4Decodes::FillBlackHashTable(oneCode *i_XxxBlackCodes, short i_nElem)
	{
		COMP_TRYTHIS
		short i = 0;
		short idx = 0;
		for (i = 0; i < i_nElem; i++)
		{
			idx = GetBlackIndex(i_XxxBlackCodes[i].GetCode(),
								i_XxxBlackCodes[i].GetLength());
			//#ifdef _DEBUG  // closed hash table->entry should be empty
			Assert(m_BlackHashTable[idx].GetCode() == -1, Util::CParamException());
			//#endif
			m_BlackHashTable[idx] = i_XxxBlackCodes[i];
		}
		COMP_CATCHTHIS
	}
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	void CT4Codes::TermWhiteInit(short i_count,
								 short i_code,
								 short i_length)
	{
		COMP_TRYTHIS
		TermWhiteCodes[i_count](oneCode::e_TermWhite,
								i_code,
								i_length,
								i_count);
		COMP_CATCHTHIS
	}
	//-----------------------------------------------------------------------
	void CT4Codes::TermBlackInit(short i_count,
								 short i_code,
								 short i_length)
	{
		COMP_TRYTHIS
		TermBlackCodes[i_count](oneCode::e_TermBlack,
								i_code,
								i_length,
								i_count);
		COMP_CATCHTHIS
	}
	//-----------------------------------------------------------------------
	void CT4Codes::MarkUpWhiteInit(short i_count,
								   short i_code,
								   short i_length)
	{
		COMP_TRYTHIS
		short cnt = i_count / 64 - 1;
		MarkUpWhiteCodes[cnt](oneCode::e_MarkUpWhite,
							  i_code,
							  i_length,
							  i_count);
		COMP_CATCHTHIS
	}
	//-----------------------------------------------------------------------
	void CT4Codes::MarkUpBlackInit(short i_count,
								   short i_code,
								   short i_length)
	{
		COMP_TRYTHIS
		short cnt = i_count / 64 - 1;
		MarkUpBlackCodes[cnt](oneCode::e_MarkUpBlack,
							  i_code,
							  i_length,
							  i_count);
		COMP_CATCHTHIS
	}
	//-----------------------------------------------------------------------
	void CT4Codes::MarkUpAddInit(short i_count,
								 short i_code,
								 short i_length)
	{
		COMP_TRYTHIS
		short cnt = (i_count - 1792) / 64;
		MarkUpAddCodes[cnt](oneCode::e_MarkUpAdd,
							i_code,
							i_length,
							i_count);
		COMP_CATCHTHIS
	}
	//-----------------------------------------------------------------------
	CT4Codes::CT4Codes()
	{
		COMP_TRYTHIS
		//-------------------------------------------------------------------
		TermWhiteInit(0, 0x35, 8);
		TermWhiteInit(1, 0x7, 6);
		TermWhiteInit(2, 0x7, 4);
		TermWhiteInit(3, 0x8, 4);
		TermWhiteInit(4, 0xb, 4);
		TermWhiteInit(5, 0xc, 4);
		TermWhiteInit(6, 0xe, 4);
		TermWhiteInit(7, 0xf, 4);
		TermWhiteInit(8, 0x13, 5);
		TermWhiteInit(9, 0x14, 5);
		TermWhiteInit(10, 0x7, 5);
		TermWhiteInit(11, 0x8, 5);
		TermWhiteInit(12, 0x8, 6);
		TermWhiteInit(13, 0x3, 6);
		TermWhiteInit(14, 0x34, 6);
		TermWhiteInit(15, 0x35, 6);
		TermWhiteInit(16, 0x2a, 6);
		TermWhiteInit(17, 0x2b, 6);
		TermWhiteInit(18, 0x27, 7);
		TermWhiteInit(19, 0xc, 7);
		TermWhiteInit(20, 0x8, 7);
		TermWhiteInit(21, 0x17, 7);
		TermWhiteInit(22, 0x3, 7);
		TermWhiteInit(23, 0x4, 7);
		TermWhiteInit(24, 0x28, 7);
		TermWhiteInit(25, 0x2b, 7);
		TermWhiteInit(26, 0x13, 7);
		TermWhiteInit(27, 0x24, 7);
		TermWhiteInit(28, 0x18, 7);
		TermWhiteInit(29, 0x2, 8);
		TermWhiteInit(30, 0x3, 8);
		TermWhiteInit(31, 0x1a, 8);
		TermWhiteInit(32, 0x1b, 8);
		TermWhiteInit(33, 0x12, 8);
		TermWhiteInit(34, 0x13, 8);
		TermWhiteInit(35, 0x14, 8);
		TermWhiteInit(36, 0x15, 8);
		TermWhiteInit(37, 0x16, 8);
		TermWhiteInit(38, 0x17, 8);
		TermWhiteInit(39, 0x28, 8);
		TermWhiteInit(40, 0x29, 8);
		TermWhiteInit(41, 0x2a, 8);
		TermWhiteInit(42, 0x2b, 8);
		TermWhiteInit(43, 0x2c, 8);
		TermWhiteInit(44, 0x2d, 8);
		TermWhiteInit(45, 0x4, 8);
		TermWhiteInit(46, 0x5, 8);
		TermWhiteInit(47, 0xa, 8);
		TermWhiteInit(48, 0xb, 8);
		TermWhiteInit(49, 0x52, 8);
		TermWhiteInit(50, 0x53, 8);
		TermWhiteInit(51, 0x54, 8);
		TermWhiteInit(52, 0x55, 8);
		TermWhiteInit(53, 0x24, 8);
		TermWhiteInit(54, 0x25, 8);
		TermWhiteInit(55, 0x58, 8);
		TermWhiteInit(56, 0x59, 8);
		TermWhiteInit(57, 0x5a, 8);
		TermWhiteInit(58, 0x5b, 8);
		TermWhiteInit(59, 0x4a, 8);
		TermWhiteInit(60, 0x4b, 8);
		TermWhiteInit(61, 0x32, 8);
		TermWhiteInit(62, 0x33, 8);
		TermWhiteInit(63, 0x34, 8);
		//-------------------------------------------------------------------
		MarkUpWhiteInit(64, 0x1b, 5);
		MarkUpWhiteInit(128, 0x12, 5);
		MarkUpWhiteInit(192, 0x17, 6);
		MarkUpWhiteInit(256, 0x37, 7);
		MarkUpWhiteInit(320, 0x36, 8);
		MarkUpWhiteInit(384, 0x37, 8);
		MarkUpWhiteInit(448, 0x64, 8);
		MarkUpWhiteInit(512, 0x65, 8);
		MarkUpWhiteInit(576, 0x68, 8);
		MarkUpWhiteInit(640, 0x67, 8);
		MarkUpWhiteInit(704, 0xcc, 9);
		MarkUpWhiteInit(768, 0xcd, 9);
		MarkUpWhiteInit(832, 0xd2, 9);
		MarkUpWhiteInit(896, 0xd3, 9);
		MarkUpWhiteInit(960, 0xd4, 9);
		MarkUpWhiteInit(1024, 0xd5, 9);
		MarkUpWhiteInit(1088, 0xd6, 9);
		MarkUpWhiteInit(1152, 0xd7, 9);
		MarkUpWhiteInit(1216, 0xd8, 9);
		MarkUpWhiteInit(1280, 0xd9, 9);
		MarkUpWhiteInit(1344, 0xda, 9);
		MarkUpWhiteInit(1408, 0xdb, 9);
		MarkUpWhiteInit(1472, 0x98, 9);
		MarkUpWhiteInit(1536, 0x99, 9);
		MarkUpWhiteInit(1600, 0x9a, 9);
		MarkUpWhiteInit(1664, 0x18, 6);
		MarkUpWhiteInit(1728, 0x9b, 9);
		//-------------------------------------------------------------------
		TermBlackInit(0, 0x37, 10);
		TermBlackInit(1, 0x2, 3);
		TermBlackInit(2, 0x3, 2);
		TermBlackInit(3, 0x2, 2);
		TermBlackInit(4, 0x3, 3);
		TermBlackInit(5, 0x3, 4);
		TermBlackInit(6, 0x2, 4);
		TermBlackInit(7, 0x3, 5);
		TermBlackInit(8, 0x5, 6);
		TermBlackInit(9, 0x4, 6);
		TermBlackInit(10, 0x4, 7);
		TermBlackInit(11, 0x5, 7);
		TermBlackInit(12, 0x7, 7);
		TermBlackInit(13, 0x4, 8);
		TermBlackInit(14, 0x7, 8);
		TermBlackInit(15, 0x18, 9);
		TermBlackInit(16, 0x17, 10);
		TermBlackInit(17, 0x18, 10);
		TermBlackInit(18, 0x8, 10);
		TermBlackInit(19, 0x67, 11);
		TermBlackInit(20, 0x68, 11);
		TermBlackInit(21, 0x6c, 11);
		TermBlackInit(22, 0x37, 11);
		TermBlackInit(23, 0x28, 11);
		TermBlackInit(24, 0x17, 11);
		TermBlackInit(25, 0x18, 11);
		TermBlackInit(26, 0xca, 12);
		TermBlackInit(27, 0xcb, 12);
		TermBlackInit(28, 0xcc, 12);
		TermBlackInit(29, 0xcd, 12);
		TermBlackInit(30, 0x68, 12);
		TermBlackInit(31, 0x69, 12);
		TermBlackInit(32, 0x6a, 12);
		TermBlackInit(33, 0x6b, 12);
		TermBlackInit(34, 0xd2, 12);
		TermBlackInit(35, 0xd3, 12);
		TermBlackInit(36, 0xd4, 12);
		TermBlackInit(37, 0xd5, 12);
		TermBlackInit(38, 0xd6, 12);
		TermBlackInit(39, 0xd7, 12);
		TermBlackInit(40, 0x6c, 12);
		TermBlackInit(41, 0x6d, 12);
		TermBlackInit(42, 0xda, 12);
		TermBlackInit(43, 0xdb, 12);
		TermBlackInit(44, 0x54, 12);
		TermBlackInit(45, 0x55, 12);
		TermBlackInit(46, 0x56, 12);
		TermBlackInit(47, 0x57, 12);
		TermBlackInit(48, 0x64, 12);
		TermBlackInit(49, 0x65, 12);
		TermBlackInit(50, 0x52, 12);
		TermBlackInit(51, 0x53, 12);
		TermBlackInit(52, 0x24, 12);
		TermBlackInit(53, 0x37, 12);
		TermBlackInit(54, 0x38, 12);
		TermBlackInit(55, 0x27, 12);
		TermBlackInit(56, 0x28, 12);
		TermBlackInit(57, 0x58, 12);
		TermBlackInit(58, 0x59, 12);
		TermBlackInit(59, 0x2b, 12);
		TermBlackInit(60, 0x2c, 12);
		TermBlackInit(61, 0x5a, 12);
		TermBlackInit(62, 0x66, 12);
		TermBlackInit(63, 0x67, 12);
		//-------------------------------------------------------------------
		MarkUpBlackInit(64, 0xf, 10);
		MarkUpBlackInit(128, 0xc8, 12);
		MarkUpBlackInit(192, 0xc9, 12);
		MarkUpBlackInit(256, 0x5b, 12);
		MarkUpBlackInit(320, 0x33, 12);
		MarkUpBlackInit(384, 0x34, 12);
		MarkUpBlackInit(448, 0x35, 12);
		MarkUpBlackInit(512, 0x6c, 13);
		MarkUpBlackInit(576, 0x6d, 13);
		MarkUpBlackInit(640, 0x4a, 13);
		MarkUpBlackInit(704, 0x4b, 13);
		MarkUpBlackInit(768, 0x4c, 13);
		MarkUpBlackInit(832, 0x4d, 13);
		MarkUpBlackInit(896, 0x72, 13);
		MarkUpBlackInit(960, 0x73, 13);
		MarkUpBlackInit(1024, 0x74, 13);
		MarkUpBlackInit(1088, 0x75, 13);
		MarkUpBlackInit(1152, 0x76, 13);
		MarkUpBlackInit(1216, 0x77, 13);
		MarkUpBlackInit(1280, 0x52, 13);
		MarkUpBlackInit(1344, 0x53, 13);
		MarkUpBlackInit(1408, 0x54, 13);
		MarkUpBlackInit(1472, 0x55, 13);
		MarkUpBlackInit(1536, 0x5a, 13);
		MarkUpBlackInit(1600, 0x5b, 13);
		MarkUpBlackInit(1664, 0x64, 13);
		MarkUpBlackInit(1728, 0x65, 13);
		//-------------------------------------------------------------------
		MarkUpAddInit(1792, 0x8, 11);
		MarkUpAddInit(1856, 0xc, 11);
		MarkUpAddInit(1920, 0xd, 11);
		MarkUpAddInit(1984, 0x12, 12);
		MarkUpAddInit(2048, 0x13, 12);
		MarkUpAddInit(2112, 0x14, 12);
		MarkUpAddInit(2176, 0x15, 12);
		MarkUpAddInit(2240, 0x16, 12);
		MarkUpAddInit(2304, 0x17, 12);
		MarkUpAddInit(2368, 0x1c, 12);
		MarkUpAddInit(2432, 0x1d, 12);
		MarkUpAddInit(2496, 0x1e, 12);
		MarkUpAddInit(2560, 0x1f, 12);
		//-------------------------------------------------------------------
		COMP_CATCHTHIS
	}
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
} // end namespace COMP
