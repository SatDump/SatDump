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

#ifndef CImage_included
#define CImage_included

/*******************************************************************************

TYPE:
Concrete class

PURPOSE: 
Provide support for image-based data manipulation.

FUNCTION:	
An image is a data buffer of a non trivial (i.e. architecture dependent) datatype 
(in this case short) together with size information. The functions supported are
requantization, point-transform and packing, together with more basic access functions.


INTERFACES:	
See 'INTERFACES' in the module declaration below 
      
RESOURCES:	
Heap Memory (>2K).
      
REFERENCE:	
None.

PROCESSING:	
The constructor takes either a CDataFieldUncompressed buffer or a size information
as input. The necessary memory (image buffer) is then allocated. In the case a 
CDataFieldUncompressed buffer was passed, the imagebuffer is filled with the data
present in the CDataField, eventually unpacked.
The functions put_block() and get_block() respectively insert/extract a 8x8 pixel
block from the image at the specified location. Ihe private function pad_block()
is used in order to support put/get operations across the edge of the image (in case
the dimension of the image wouldn't be a multiple of 8).
The member functions Forward_point_transform() and Inverse_point_transform() perform 
the "point-transform" (shifting of the image data value).
The member function Pack() returns the current image, packed according to the specified
parameter in a CDataFieldUncompressedImage;
The member function Requantize() performs data requantization as specified in the MSI. The
m_NB data member is modified accordingly.
Various miscelaneous functions (operator()(), GetNext(), SetNext()) allow access
to individual pixel values. ResetState() performs a reset of the internal
position counters used by {Get|Set}Next().

DATA:
None			
      
LOGIC:
      
*******************************************************************************/

#include <vector>

#include "CDataField.h"		  // DISE
#include "CBlock.h"			  // COMP
#include "RMAErrorHandling.h" // COMP
#include "Compress.h"		  // COMP

namespace COMP
{

	class CImage
	{

	private:
		//DATA:

		//	Object variables
		std::vector<unsigned short> m_data;		 //	Pointer to the raw image data buffer.
		std::vector<unsigned short *> m_dataptr; // array of pointers to each line of the block
												 // m_dataptr[0] = m_data
												 // m_dataptr[1] = m_data + m_NC
												 // m_dataptr[2] = m_data + 2 * m_NC
												 // ...
												 // m_dataptr[n] = m_data + n * m_NC
												 // ...
												 // m_dataptr[m_NL-1] = m_data + (m_NL - 1) * m_NC
		unsigned short m_NL;					 //  Number of lines in the image
		unsigned short m_NC;					 //  Number of columns in the image
		unsigned short m_NB;					 //  Max number of valid bits of the current data
		unsigned long m_size;					 //  Number of pixels in the image.
		//	State variables: used to store object state information
		unsigned long m_index;	 //	Linear position in the buffer
		unsigned short m_cur_L;	 //	Current line position
		unsigned short m_cur_C;	 //	Current column position in the buffer
		unsigned short m_cur_RI; //	Current Restart Interval index
		unsigned short m_RI;	 //  Current Restart interval length (in pixels)

		//INTERFACE:

		// Description:	Performs the block padding for an incomplete block.
		// Returns:		Nothing.
		void pad_block(
			CBlock<unsigned short> &t_blk,	 // block to be padded
			const unsigned short &i_goodCol, // # of valid columns
			const unsigned short &i_goodLine // # of valid lines
		)
			const
		{
			COMP_TRYTHIS
			// Pads missing pixels by copying last column, last line
			// 1� Copying columns
			unsigned int tmp = i_goodCol - 1;
			for (unsigned int j = i_goodCol; j < 8; j++)
				for (unsigned int i = 0; i < i_goodLine; i++)
					t_blk.Cset(i, j, t_blk.Cget(i, tmp));

			// 2� Copying lines
			tmp = i_goodLine - 1;
			for (unsigned int i = i_goodLine; i < 8; i++)
				for (unsigned int j = 0; j < 8; j++)
					t_blk.Cset(i, j, t_blk.Cget(tmp, j));
			COMP_CATCHTHIS
		}

	public:
		// Description:	Creates a CImage from size information.
		//				This is also the default constructor.
		// Returns:		Nothing.
		CImage(
			unsigned short i_NC = 0, // Number of columns of the image to create
			unsigned short i_NL = 0, // Number of lines of the image to create
			unsigned short i_NB = 16 // "Depth" of the image to create (is used in the pack() member)
		);

		// Description:	Creates a CImage from the CDataFieldUncompressedImage by unpacking it.
		// Returns:		Nothing.
		CImage(
			const Util::CDataFieldUncompressedImage &i_cdfui // buffer containing the image data.
															 // May be packed.
		);

		// Description:	Destructor
		// Returns:		Nothing.
		~CImage()
		{
		}

		// Description:	Recreates a CImage from size information.
		//				Existing information is discarted.
		// Returns:		Nothing.
		void Resize(
			unsigned short i_NC,	 // Number of columns of the image to create
			unsigned short i_NL,	 // Number of lines of the image to create
			unsigned short i_NB = 16 // "Depth" of the image to create (is used in the pack() member)
		);

		// Description:	Place the given pixel 8x8 block at the specified position.
		// Returns:		Nothing.
		void put_block(
			CBlock<unsigned short> &i_blk, // pixel block to insert
			unsigned short i_colLeft,	   // position of the upper left pixel of the block
			unsigned short i_lineUp		   // "
		)
		{
			COMP_TRYTHIS
			// Puts a 8*8 pixels block into the buffer at the specified position
			unsigned int bStep, iStep;
			unsigned int stop_line = i_lineUp + 8;
			unsigned int stop_column = i_colLeft + 8;

#ifdef DEBUG
			Assert(i_lineUp < m_NL, Util::CParamException());
			Assert(i_colLeft < m_NC, Util::CParamException());
#endif

			// check for image limits
			if (stop_line > m_NL)
				stop_line = m_NL;
			if (stop_column > m_NC)
			{
				stop_column = m_NC;
				bStep = 8 - stop_column + i_colLeft;
				iStep = m_NC - 8 + bStep;
			}
			else
			{
				bStep = 0;
				iStep = m_NC - 8;
			}

			unsigned int maxval = (1 << m_NB) - 1;
			unsigned short val = 0;

			// copy the block into the buffer
			unsigned int bIndex = 0;
			unsigned long iIndex = (unsigned long)i_lineUp * m_NC + i_colLeft;
			for (unsigned int i = i_lineUp; i < stop_line; i++, bIndex += bStep, iIndex += iStep)
				for (unsigned int j = i_colLeft; j < stop_column; j++)
					//m_data[iIndex++] = i_blk.Cget (bIndex++);
					m_data[iIndex++] = ((val = i_blk.Cget(bIndex++)) > maxval) ? maxval : val;
			COMP_CATCHTHIS
		}

		// Description:	Extract a pixel block at the specified position
		// Returns:		Nothing.
		void get_block(
			CBlock<unsigned short> &o_blk, //
			unsigned short i_colLeft,	   // position of the upper left pixel of the block
			unsigned short i_lineUp		   // "
		)
			const
		{
			COMP_TRYTHIS
			// Takes a 8*8 pixels block into the buffer at the specified position,
			// and pads it if needed.

			unsigned int bStep, iStep;
			unsigned int stop_line = i_lineUp + 8;
			unsigned int stop_column = i_colLeft + 8;
			unsigned int real_lines = 8;   // number of correct lines
			unsigned int real_columns = 8; // number of correct columns
			bool padding = false;

#ifdef DEBUG
			Assert(i_lineUp < m_NL, Util::CParamException());
			Assert(i_colLeft < m_NC, Util::CParamException());
#endif

			// Test for image boundaries
			if (stop_line > m_NL)
			{
				padding = true;
				stop_line = m_NL;
				real_lines = stop_line - i_lineUp;
			}
			if (stop_column > m_NC)
			{
				padding = true;
				stop_column = m_NC;
				real_columns = stop_column - i_colLeft;
				bStep = 8 - real_columns;
				iStep = m_NC - 8 + bStep;
			}
			else
			{
				bStep = 0;
				iStep = m_NC - 8;
			}

			// 1� Take real pixels

			unsigned int bIndex = 0;
			unsigned long iIndex = (unsigned long)i_lineUp * m_NC + i_colLeft;
			for (unsigned int i = i_lineUp; i < stop_line; i++, bIndex += bStep, iIndex += iStep)
				for (unsigned int j = i_colLeft; j < stop_column; j++)
					o_blk.Cset(bIndex++, m_data[iIndex++]);

			// 2� Padding, if needed
			if (padding)
				pad_block(o_blk, real_columns, real_lines);
			COMP_CATCHTHIS
		}

		// Description:	Performs the forward point transform on the data
		//				as specified in the JPEG norm.
		// Returns:		Nothing.
		void Forward_point_transform(
			unsigned short i_pt // point transform
		);

		// Description:	Performs the inverse point transform on the data
		//				as specified in the JPEG norm
		// Returns:		Nothing.
		void Inverse_point_transform(
			unsigned short i_pt // point transform
		);

		// Description: Packs an uncompressed image, from 16 bits/pixel
		//				to the specified representation.
		// Returns:		The packed uncompressed image.
		Util::CDataFieldUncompressedImage pack(
			unsigned short i_NR // number of representation bits.
		);

		// Description:	Perform data requantization (12|10 bit-> 8bit) as specified in the MSI.
		//				Note that this function is supposed to also modify the m_NB to reflect
		//				the change.
		// Returns:		Nothing.
		void Requantize(
			COMP::ERequantizationMode i_mode // specifies the chosen requantization mode.
		);

		// Description:	Returns a reference to the specified pixel value.
		// Returns:		The pixel value at the specified location.
		unsigned short &operator()(
			const unsigned short &i_i, // line number of the pixel value to return
			const unsigned short &i_j  // column number of the pixel value to return
		)
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(((unsigned long)i_i * m_NC + i_j) < m_size, Util::CParamException());
#endif
			return m_dataptr[i_i][i_j];
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Returns the value of the specified pixel.
		// Returns:		The pixel value at the specified location.
		unsigned short Cget(
			const unsigned short &i_i, // line number of the pixel value to return
			const unsigned short &i_j  // column number of the pixel value to return
		)
			const
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(((unsigned long)i_i * m_NC + i_j) < m_size, Util::CParamException());
#endif
			return m_dataptr[i_i][i_j];
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Set the value of the specified pixel.
		// Returns:		Nothing.
		void Cset(
			const unsigned short &i_i,	// line number of the pixel value to return
			const unsigned short &i_j,	// column number of the pixel value to return
			const unsigned short &i_val // the pixel value
		)
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(((unsigned long)i_i * m_NC + i_j) < m_size, Util::CParamException());
#endif
			m_dataptr[i_i][i_j] = i_val;
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Returns a reference to the specified pixel value.
		// Returns:		The pixel value at the specified location.
		unsigned short &operator[](
			const unsigned long &i_i // linear pixel index to return
		)
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(i_i < m_size, Util::CParamException());
#endif
			return m_data[i_i];
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Returns the value of the specified pixel.
		// Returns:		The pixel value at the specified location.
		unsigned short Cget(
			const unsigned long &i_i // the linear pixel index
		)
			const
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(i_i < m_size, Util::CParamException());
#endif
			return m_data[i_i];
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Set the value of the specified pixel
		// Returns:		Nothing.
		void Cset(
			const unsigned long &i_i,	// the linear pixel index
			const unsigned short &i_val // the pixel value
		)
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(i_i < m_size, Util::CParamException());
#endif
			m_data[i_i] = i_val;
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Put the given value at the current index position and increment
		//				the current index position. Index position are counted in a linear
		//				manner in the image buffer
		// Returns:		Nothing.
		void SetNext(
			const unsigned short &i_v // new value of the pixel.
		)
		{
			COMP_TRYTHIS
#ifdef _DEBUG
			Assert(m_index < m_size, COMP::COutOfBufferException());
#endif
			m_data[m_index++] = i_v;
			COMP_CATCHTHIS
		}

		// Description:	Returns the pixel value at the current index position and increment
		//				the current index position.
		// Returns:		The specified pixel value
		unsigned short GetNext()
		{
			COMP_TRYTHIS
#ifdef _DEBUG
			Assert(m_index < m_size, COMP::COutOfBufferException());
#endif
			return m_data[m_index++];
			COMP_CATCHTHIS
		}

		// Description:	Reset the state information (m_index, m_cur_*, ...) to the initial
		//				buffer position.
		// Returns:		Nothing.
		void ResetState();

		// Description:	Returns the image height.
		// Returns:		The number of lines in the image.
		unsigned short GetH() const { return m_NL; }

		// Description:	Returns the image width.
		// Returns:		The number of columns in the image.
		unsigned short GetW() const { return m_NC; }

		// Description:	Returns the image depth.
		// Returns:		The number of used bits per pixel.
		unsigned short GetNB() const { return m_NB; }

		// Description:	Similar to std::auto_ptr::size().
		// Returns:		The number of pixels in the image.
		unsigned long GetSize() const { return m_size; }

		// Description:	Similar to std::auto_ptr::get().
		// Returns:		A pointer to the raw data.
		unsigned short *Get() { return &m_data[0]; }

		// Description:	Gets the array of pointers to lines.
		// Returns:		A pointer to the array of line pointers.
		unsigned short **GetP() { return &m_dataptr[0]; }

		// Description:	Set the pixels of the lines from line # i_from to line
		//				# i_to included to 0.
		//				The current index position is updated to point at the first
		//				pixel following the last pixel reset.
		// Returns:		Nothing.
		void Zero(
			const unsigned short &i_from, // first line to reset
			const unsigned short &i_to	  // last line to reset
		)
		{
			COMP_TRYTHIS
			const unsigned long x_from = i_from * m_NC;
			const unsigned long x_to = i_to * m_NC;
#ifdef _DEBUG
			if (x_from != m_index)
				std::cerr << "!!! x_from != m_index\n";
			Assert(x_from == m_index, Util::CParamException());
			Assert(x_to <= m_size, Util::CParamException());
#endif
			for (unsigned long i = x_from; i < x_to; i++)
				m_data[i] = 0;
			m_index = x_to;
			COMP_CATCHTHIS
		}

		// Description:	Set the pixels between the current index and the specified index to 0.
		//				The current index position is updated.
		// Returns:		Nothing.
		void ZeroPixels(
			const unsigned long &i_from, // index of the first pixel to reset.
			const unsigned long &i_to	 // index of the last pixel to reset.
		)
		{
			COMP_TRYTHIS
#ifdef _DEBUG
			Assert(i_from <= i_to && i_to < m_size, Util::CParamException());
#endif
			for (m_index = i_from; m_index <= i_to;)
				m_data[m_index++] = 0;
			COMP_CATCHTHIS
		}
	};

} // end namespace

#endif
