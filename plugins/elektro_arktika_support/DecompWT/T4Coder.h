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

#ifndef T4Coder_included
#define T4Coder_included

/*******************************************************************************

TYPE:
Concrete class.
      
PURPOSE:
Encapsulate the coding of a binary image in T.4 format.
      
FUNCTION:
class CT4Coder
Holds the compressed & uncompressed buffers and performs the
needed operations to compress the uncompressed buffer.
      
INTERFACES:
See 'INTERFACES' in the module declaration below
      
RESOURCES:	
Heap Memory (>2K).
      
REFERENCE:	
none

PROCESSING:	
After constructing the object with a uncompressed image (the
constructor allocates the compressed buffer -- per default, the
allocated output buffer has the same size as the input buffer ---
the	size of the output buffer in increased by a factor 2 each time
a buffer overflow occurs), the function CodeBuffer() is supposed
to be called, which in turn will call CodeNexLine() (wich will call
CodeRunLength() to perform the actual coding of one run) and CodeEOL()
(that writes the T.4 EOL code).
GetCompressedImage() must then be used to obtain the compressed data 
field.
      
DATA:
See 'Data' in the class header below.			
      
LOGIC:
-

*******************************************************************************/

#include <string>

#include "CDataField.h"
#include "CBitBuffer.h"
#include "T4Codes.h"

namespace COMP
{

	class CT4Coder
	{

	private:
		// DATA:

		// Color constants.
		enum EColor
		{
			e_Black = false,
			e_White = true,
			e_lastColor
		};

		CT4Codes m_AllCodes; // the T4 codes & associated routines
		short m_height;		 // size of the image
		short m_width;
		CBitBuffer m_ibuf;				  // input buffer (uncompressed image) (packed)
		CBitBuffer m_obuf;				  // output buffer (compressed buffer)
		unsigned long m_compressedlength; // length of the valid data in m_obuf

		// INTERFACE:

		// Description:	Writes an EOL at the current buffer position.
		// Returns:		Nothing.
		void CodeEOL()
		{
			COMP_TRYTHIS
			m_obuf.WriteLSb(0x0001, 12);
			COMP_CATCHTHIS
		}

		// Description:	Code a Run of the specified length & color.
		// Returns:		Nothing.
		void CodeRunLength(
			EColor i_isWhite, // true  if the run to code correspond to "set"(=1) pixels,
							  // false if the run to code correspond to "reset"(=0) pixels.
			short i_count	  // length of the run to code.
		);

		// Description:	Code one image line (inc line EOL).
		// Returns:		Nothing.
		void CodeNextLine();

	public:
		// Description:	Constructor. Initialise the coder with its input data and allocate
		//				the output buffer.
		// Returns:		Nothing.
		CT4Coder(
			const Util::CDataFieldUncompressedImage &i_datafield // uncompressed input image datafield.
																 // MUST have a NR & NB ==1.
			)
			: m_ibuf(i_datafield), m_obuf((unsigned long)i_datafield.GetNC() * i_datafield.GetNL() * i_datafield.GetNR())
		{
			COMP_TRYTHIS
			Assert(i_datafield.GetNR() == 1, Util::CParamException());
			Assert(i_datafield.GetNB() == 1, Util::CParamException());
			m_height = i_datafield.GetNL();
			m_width = i_datafield.GetNC();
			m_compressedlength = 0;
			COMP_CATCHTHIS
		}

		// Description:	Destructor.
		// Returns:		Nothing.
		~CT4Coder()
		{
		}

		// Description:	Performs the coding of the input buffer in the output buffer.
		// Returns:		Nothing.
		void CodeBuffer();

		// Description:	Function to call to receive the compressed image stream.
		// Returns:		A Util::CDataFieldCompressedImage containing the compressed
		//				image stream.
		Util::CDataFieldCompressedImage GetCompressedImage();
	};

} // end namespace

#endif
