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

#ifndef CQualityInfo_included
#define CQualityInfo_included

/*******************************************************************************

TYPE:
Concrete class

PURPOSE: 
Provide support for quality-information data manipulation.

FUNCTION:	
This object encapsulates methods to manipulate the quality information
vector such as SetQuality(), ...

INTERFACES:	
See 'INTERFACES' in the module declaration below 
      
RESOURCES:	
Heap Memory (>2K).
      
REFERENCE:	
None.

PROCESSING:	
The object is created using the specified size information..
The method SetNextLines() is used to set the quality information
of several lines at once.

DATA:
None			
      
LOGIC:
-

*******************************************************************************/

#include <memory>
#include <stdlib.h>

#include "CDataField.h"
#include "RMAErrorHandling.h"

namespace COMP
{

	class CQualityInfo : public std::vector<short>
	{

	private:
		// DATA:

		unsigned short m_cur_L; //	Current line position

	public:
		// INTERFACES:

		// Description:	Creates a CQInfo from the size information contained in
		//				the CDataFieldUncompressedImage .
		// Returns:		Nothing.
		CQualityInfo(
			const Util::CDataFieldCompressedImage &i_cdfui // buffer containing the image data.
														   // Only the image size information is used.
			)
			: std::vector<short>(i_cdfui.GetNL())
		{
			COMP_TRYTHIS
			for (unsigned short i = 0; i < size(); i++)
				(*this)[i] = 0;
			m_cur_L = 0; // reset the current position
			COMP_CATCHTHIS
		}

		// Description:	Destructor.
		// Returns:		Nothing.
		~CQualityInfo()
		{
		}

		// Description:	Set the quality of the specified line at the specified value.
		// Returns:		Nothing.
		void Set(
			const unsigned short &i_line, // line to be set
			const short &i_quality		  // value to store in the vector.
		)
		{
			COMP_TRYTHIS
#ifdef _DEBUG
			Assert(i_line < size(), Util::CParamException());
#endif
			(*this)[i_line] = i_quality;
			COMP_CATCHTHIS
		}

		// Description:	Set the quality of the specified lines at the specified value.
		// Returns:		Nothing.
		void Set(
			const unsigned short &i_from, // first line to be set
			const unsigned short &i_to,	  // last line to be set
			const short &i_quality		  // value to store in the vector.
		)
		{
			COMP_TRYTHIS
#ifdef _DEBUG
			Assert(i_to < size(), Util::CParamException());
#endif
			for (unsigned short i = i_from; i <= i_to; i++)
				(*this)[i] = i_quality;
			COMP_CATCHTHIS
		}

		// Description:	Set the quality of the lines from line # i_from to line
		//				# i_to included to 0.
		// Returns:		Nothing.
		void Zero(
			const unsigned short &i_from, // first line to reset
			const unsigned short &i_to	  // last line to reset
		)
		{
			COMP_TRYTHIS
#ifdef _DEBUG
			Assert(i_to <= size(), Util::CParamException());
#endif
			for (unsigned short i = i_from; i < i_to; i++)
				(*this)[i] = 0;
			COMP_CATCHTHIS
		}

		// Description:	Negate the quality of the lines from line # i_from to line
		//				# i_to included to 0.
		// Returns:		Nothing.
		void Negate(
			const unsigned short &i_from, // first line to negate
			const unsigned short &i_to	  // last line to negate
		)
		{
			COMP_TRYTHIS
#ifdef _DEBUG
			Assert(i_to < size(), Util::CParamException());
#endif
			for (unsigned short i = i_from; i <= i_to; i++)
				(*this)[i] = -abs((*this)[i]);
			COMP_CATCHTHIS
		}
	};

} // end namespace

#endif
