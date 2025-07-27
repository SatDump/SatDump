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

#ifndef T4Codes_included
#define T4Codes_included

/*******************************************************************************

TYPE:	Concrete Class

PURPOSE:	
These classes encapsulate methods to handle the T.4 specific Huffmann codes.

FUNCTION:	
class oneCode
	Holds together the information relative to one particular
	T.4 Huffman code.
class CT4Codes
	Initialize the tables containing the T.4 Huffman codes
	This class provides functions for easy conversion of a
	given run length to the desired T.4 Huffmann code.
class CT4Decodes
	This class provides functions for easy conversion of
	a given T.4 Huffman code in the corresponding run
	length. 
      
INTERFACES:	
See 'INTERFACES' in the module declaration below
      
RESOURCES:	
Heap Memory (>2K).
      
REFERENCE:	
The T.4 codes are defined in the Red Book , Volume VII,
Fascicle VII.3, Terminal Equipment and Protocols for Telematic
Services, The International Telegraph and Telephone Consultative
Committee (CCITT), Geneva, 1985.

PROCESSING:	
class oneCode:
	This class should not be directly used. Its intend
	is to support the derived classes.
	The constructors initialises the data members
	(m_codeType, m_code, m_length and m_count) to
	default values. 
	The member operator()() is used to modify these
	default values. The rationale for the absence of
	a constructor able to assign useful values to the
	data members, is that this class will always be constructed
	as part of an array.
	The members Get*() provide read-access to the otherwise private data members.
      
class CT4Codes
	Upon construction, this class creates 5 arrays (TermWhiteCodes,
	TermBlackCodes, MarkUpWhiteCodes, MarkUpBlackCodes, MarkUpAddCodes)
	of oneCodes (one for each category of T.4 code). The size of these
	arrays is given by the constants e_N*Codes.
	Each element of these arrays is then initialized with the T.4 code/length
	by calling the corresponding member function (TermWhiteInit(), TermBlackInit(),
	MarkUpWhiteInit(), MarkUpBlackInit(), MarkUpAddInit()) with the required parameters.
	The T.4 code corresponding to a given run length, in a known context (white/black
	run, terminating/markup/additional code) can be retrieved by calling 
	respectively GetTermWhiteCode(), GetTermBlackCode(), GetMarkUpWhiteCode(), 
	GetMarkUpBlackCode(), GetMarkUpAddCode().
		
class CT4Decodes
	The lookup done by the member of this class are implemented by using a
	closed hash table (see GetIndex() for the hash function).
	Upon construction, this class initialises the hash tables (m_WhiteHashTable,
	m_BlackHashTable) used to find the codes back. The private members 
	FillWhiteHashTable() and FillBlackHashTable() are used during this initialisation
	process. These member functions call the hash functions (GetWhiteIndex(),
	GetBlackIndex() ( wich in turn call GetIndex()) that return the index of the code 
	in the hash table in function of the given code. 
	The member function that returns the run length in function of the
	code is GetCount().

DATA:
See 'Data' in the class header below.			
      
LOGIC:
      
*******************************************************************************/

#include "RMAErrorHandling.h"

namespace COMP
{

	// Contains one T.4 code (linking the Huffmann code and the run length)
	class oneCode
	{

	public:
		// DATA:

		//	These constants are used to characterise the context in which the code
		//	should be used.
		enum ECodeType
		{
			e_TermWhite,
			e_MarkUpWhite,
			e_TermBlack,
			e_MarkUpBlack,
			e_MarkUpAdd,
			e_lastCodeType
		};

	protected:
		// DATA:

		// Characterises the context in which the code should be used.
		ECodeType m_codeType;
		// contains the Huffmann code
		short m_code;
		// contains the length (number of significant bits (from the left) in m_code
		short m_length;
		// contains the length of the run associated with the code
		short m_count;

	public:
		// INTERFACE:

		// Description:	Constructor.
		// Returns:		Nothing.
		oneCode()
		{
			COMP_TRYTHIS
			m_codeType = e_lastCodeType;
			m_code = -1;
			m_length = -1;
			m_count = -1;
			COMP_CATCHTHIS
		}

		// Description:	Destructor.
		// Returns:		Nothing.
		~oneCode()
		{
		}

		// Description:	Modify the private data members using the given parameters.
		// Returns:		Nothing.
		void operator()(
			ECodeType i_codeType, // New code type to assign to the object data member.
			short i_code,		  // New Huffmann code to assign to the object data member.
			short i_length,		  // New length of Huffmann code to assign to the object data member.
			short i_count		  //
		)
		{
			COMP_TRYTHIS
			m_codeType = i_codeType;
			m_code = i_code;
			m_length = i_length;
			m_count = i_count;
			COMP_CATCHTHIS
		}

		// Description:	Return the number of count (run lengths) of the code.
		// Returns:		The run length associated with the code.
		short GetCount() const { return m_count; }

		// Description:	Returns the length of the Huffman code.
		// Returns:		The length of the Huffman code.
		short GetLength() const { return m_length; }

		// Description:	Returns the Huffman code.
		// Returns:		The the Huffman code.
		short GetCode() const { return m_code; }
	};

	class CT4Codes
	{

	protected:
		// DATA:

		//	Constants used to hold the size of the corrsponding tables (defined in
		//	the T.4 specs).
		enum EConst
		{
			e_NTermWhiteCodes = 64,
			e_NTermBlackCodes = 64,
			e_NMarkUpWhiteCodes = 27,
			e_NMarkUpBlackCodes = 27,
			e_NMarkUpAddCodes = 13
		};

		oneCode TermWhiteCodes[e_NTermWhiteCodes];	   // Table of Terminating White codes.
		oneCode TermBlackCodes[e_NTermBlackCodes];	   // Table of Terminating Black codes.
		oneCode MarkUpWhiteCodes[e_NMarkUpWhiteCodes]; // Table of MarkUp White codes.
		oneCode MarkUpBlackCodes[e_NMarkUpBlackCodes]; // Table of MarkUp Black codes.
		oneCode MarkUpAddCodes[e_NMarkUpAddCodes];	   // Table of Additional MarkUp codes.

		// INTERFACE:

		// Description:	Initialise the T.4 code (specified in i_count) with the given parameters.
		// Arguments:	i_count:	run length of the code to insert in the table.
		//				i_code:		Huffmann code of the code to insert in the table.
		//				i_length:	length of the Huffmann code of the code to insert in the table.
		// Returns:		Nothing.
		void TermWhiteInit(short i_count, short i_code, short i_length);
		void TermBlackInit(short i_count, short i_code, short i_length);
		void MarkUpWhiteInit(short i_count, short i_code, short i_length);
		void MarkUpBlackInit(short i_count, short i_code, short i_length);
		void MarkUpAddInit(short i_count, short i_code, short i_length);

	public:
		// INTERFACE:

		// Description: Constructor.
		// Returns:		Nothing.
		CT4Codes();

		// Description: Destructor.
		// Returns:		Nothing.
		~CT4Codes()
		{
		}

		// Description:	Returns the T4 code a the given index in the corresponding table
		// Arguments:	codeIdx: table index at which the code is to be fetched.
		// Returns:		The T.4 code stored at the given table index.
		oneCode &GetTermWhiteCode(short codeIdx)
		{
			COMP_TRYTHIS
#ifdef _DEBUG
			Assert(codeIdx < e_NTermWhiteCodes, Util::CNamedException("Big problem in T4 codec"));
#endif
			return TermWhiteCodes[codeIdx];
			COMP_CATCHTHIS
		}

		oneCode &GetTermBlackCode(short codeIdx)
		{
			COMP_TRYTHIS
#ifdef _DEBUG
			Assert(codeIdx < e_NTermBlackCodes, Util::CNamedException("Big problem in T4 codec"));
#endif
			return TermBlackCodes[codeIdx];
			COMP_CATCHTHIS
		}

		oneCode &GetMarkUpWhiteCode(short codeIdx)
		{
			COMP_TRYTHIS
#ifdef _DEBUG
			Assert(codeIdx < e_NMarkUpWhiteCodes, Util::CNamedException("Big problem in T4 codec"));
#endif
			return MarkUpWhiteCodes[codeIdx];
			COMP_CATCHTHIS
		}

		oneCode &GetMarkUpBlackCode(short codeIdx)
		{
			COMP_TRYTHIS
#ifdef _DEBUG
			Assert(codeIdx < e_NMarkUpBlackCodes, Util::CNamedException("Big problem in T4 codec"));
#endif
			return MarkUpBlackCodes[codeIdx];
			COMP_CATCHTHIS
		}

		oneCode &GetMarkUpAddCode(short codeIdx)
		{
			COMP_TRYTHIS
#ifdef _DEBUG
			Assert(codeIdx < e_NMarkUpAddCodes, Util::CNamedException("Big problem in T4 codec"));
#endif
			return MarkUpAddCodes[codeIdx];
			COMP_CATCHTHIS
		}
	};

	class CT4Decodes : public COMP::CT4Codes
	{

	private:
		// DATA:

		//	Constants used to hold the hash-function parameters.
		enum EConstants
		{
			e_HashTblSize = 1021, // should be a prime number for closed hash table
			e_WhiteHashA = 3510,
			e_WhiteHashB = 1178,
			e_BlackHashA = 293,
			e_BlackHashB = 2695
		};

		//	Arrays used to hold the two hash tables.
		oneCode m_WhiteHashTable[e_HashTblSize];
		oneCode m_BlackHashTable[e_HashTblSize];

		// INTERFACE:

		// Description:	Fill the hash table with the given array of codes.
		// Arguments:	i_XxxWhiteCodes: array of (Term|MarkUp|MarkUpAdd) T.4 codes
		//				                 to store in the hash table.
		//				i_bElem:         number of codes in i_XxxWhiteCodes.
		// Returns:		Nothing.
		void FillWhiteHashTable(oneCode *i_XxxWhiteCodes, short i_nElem);
		void FillBlackHashTable(oneCode *i_XxxBlackCodes, short i_nElem);

		// Description:	Computes the position in the hash table of the given code, using the
		//				specified parameters for the hash function.
		// Returns:		The index at which the Huffmann code should be stored in the hash table.
		short GetIndex(
			short i_code,	// Huffmann code to be fetched.
			short i_length, // Length to consider in i_code.
			short i_a,		// Parameters of the hash function.
			short i_b		// "
		)
		{
			COMP_TRYTHIS
			short idx = ((i_length + i_a) * (i_code + i_b)) % e_HashTblSize;
			return idx;
			COMP_CATCHTHIS
		}

	public:
		// INTERFACE:

		// Description: Constructor.
		// Returns:		Nothing.
		CT4Decodes()
		{
			COMP_TRYTHIS
			FillWhiteHashTable(TermWhiteCodes, e_NTermWhiteCodes);
			FillWhiteHashTable(MarkUpWhiteCodes, e_NMarkUpWhiteCodes);
			FillWhiteHashTable(MarkUpAddCodes, e_NMarkUpAddCodes);
			FillBlackHashTable(TermBlackCodes, e_NTermBlackCodes);
			FillBlackHashTable(MarkUpBlackCodes, e_NMarkUpBlackCodes);
			FillBlackHashTable(MarkUpAddCodes, e_NMarkUpAddCodes);
			COMP_CATCHTHIS
		}

		// Description: Destructor.
		// Returns:		Nothing.
		~CT4Decodes()
		{
		}

		// Description:	Search in the hash table the index of the code given in parameter.
		// Arguments:	i_code:   Huffmann code to be retrieved.
		//				i_length: nbr of bits to be considered in i_code .
		// Returns:		The index at which the code is to be found/stored.
		short GetWhiteIndex(short i_code, short i_length)
		{
			COMP_TRYTHIS
			return GetIndex(i_code, i_length, e_WhiteHashA, e_WhiteHashB);
			COMP_CATCHTHIS
		}

		short GetBlackIndex(short i_code, short i_length)
		{
			COMP_TRYTHIS
			return GetIndex(i_code, i_length, e_BlackHashA, e_BlackHashB);
			COMP_CATCHTHIS
		}

		// Description:	Search in the specified hash table the code given in parameter
		//				and return its runlength (if found), else return -1.
		// Returns:		The run length coded by the code (if >0), a negative value
		//				indicates that the code could not be found in the hash table.
		short GetCount(
			short i_code,	// Huffmann code to be found.
			short i_length, // nbr of bits to be considered in i_code.
			bool i_isWhite	// true if a white code is to be found, false otherwise.
		)
		{
			COMP_TRYTHIS
			short idx = -1;
			if (i_isWhite)
				idx = GetWhiteIndex(i_code, i_length);
			else
				idx = GetBlackIndex(i_code, i_length);
#ifdef _DEBUG
			Assert(idx >= 0, Util::CNamedException("negative index"));
#endif
			oneCode c;
			if (i_isWhite)
				c = m_WhiteHashTable[idx];
			else
				c = m_BlackHashTable[idx];
			if ((c.GetCode() == i_code) && (c.GetLength() == i_length))
				return c.GetCount();
			return -1;
			COMP_CATCHTHIS
		}
	};

} // end namespace

#endif
