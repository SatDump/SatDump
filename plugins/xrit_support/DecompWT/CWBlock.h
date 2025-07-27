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

#ifndef CWBlock_included
#define CWBlock_included

/*******************************************************************************

TYPE:
Concrete class.

PURPOSE:
Handle generic N*M matrix block, perform the S+P transformation.
  
FUNCTION:
This class provides functions to store and access a generic N*M elements matrix.
It allows to perform the S transform and the S+P transform.
	
INTERFACES:	
See 'INTERFACES' in the module declaration below
      
RESOURCES:	
Heap Memory (>2K).

REFERENCE:
"An Image Multiresolution Representation for Lossless and Lossy Compression",
Amir Said, William A. Pearlman. SPIE Symposium on Visual Communications and
Image Processing, Cambridge, MA, Nov. 1993

PROCESSING: 
The default	constructor allows creating an empty block (0x0 elements).
A second constructor allows creating a block with user-defined dimensions.
A Resize member functions enables to re-dimension the block.
Various public member functions allow to access private data member and to get
or store block elements.
Functions are provided to fill a block with data from a CImage object and to dump
the block data to a CImage.
Four functions allows to perfom a block multi-resolution transformation, the first 
one implements the S transform, the other ones implement the S+P transform with respectively
the A, B and C predictor (see 'REFERENCE').
The inverse transformations are also implemented.

DATA:
See 'DATA :' in the class header below.			

LOGIC:
				
*******************************************************************************/

#include <vector>

#include "RMAErrorHandling.h"
#include "Bitlevel.h"
#include "CImage.h"
#include "WTConst.h"

namespace COMP
{

	class CWBlock
	{

	private:
		// DATA:

		unsigned int m_W;		   // block width
		unsigned int m_H;		   // block height
		unsigned long m_Size;	   // block size (width * height)
		std::vector<int *> m_Data; // array of pointers to each line of the block
								   // m_Data[0] = m_Buffer
								   // m_Data[1] = m_Buffer + m_W
								   // m_Data[2] = m_Buffer + 2 * m_W
								   // ...
								   // m_Data[n] = m_Buffer + n * m_W
								   // ...
								   // m_Data[m_H-1] = m_Buffer + (mH - 1) * m_W
		std::vector<int> m_Buffer; // array of data elements
		std::vector<int> m_Tmp;	   // Temp array used during multiresolution transforms

	private:
		// INTERFACES:

		// Description:	Perform the 1D forward S transform on part of a given line.
		// Returns:		Nothing.
		void St1DH_Fwd(
			const unsigned inti_I, // line to process
			const unsigned inti_S  // width of the line
		);

		// Description: Perform the 1D forward S transform on part of a given column.
		// Returns:		Nothing.
		void St1DV_Fwd(
			const unsigned int i_I, // column to process
			const unsigned int i_S	// height of the column
		);

		// Description: Perform the 1D inverse S transform on part of a given line.
		// Returns:		Nothing.
		void St1DH_Inv(
			const unsigned int i_I, // line to process
			const unsigned int i_S	// width of the line
		);

		// Description:	Perform the 1D inverse S transform on part of a given column.
		// Returns:		Nothing.
		void St1DV_Inv(
			const unsigned int i_I, // column to process
			const unsigned int i_S	// height of the column
		);

		// Description:	Perform the 1D forward S+P transform on part of a given line
		//				using the A predictor.
		// Returns:		Nothing.
		void SptA1DH_Fwd(
			const unsigned int i_I, // line to process
			const unsigned int i_S	// width of the line
		);

		// Description:	Perform the 1D forward S+P transform on part of a given column
		//				using the A predictor.
		// Returns:		Nothing.
		void SptA1DV_Fwd(
			const unsigned int i_I, // column to process
			const unsigned int i_S	// height of the column
		);

		// Description:	perform the 1D inverse S+P transform on part of a given line
		//				using the A predictor.
		// Returns:		Nothing.
		void SptA1DH_Inv(
			const unsigned int i_I, // line to process
			const unsigned int i_S	// width of the line
		);

		// Description: Perform the 1D inverse S+P transform on part of a given column
		//				using the A predictor.
		// Returns:		Nothing.
		void SptA1DV_Inv(
			const unsigned int i_I, // column to process
			const unsigned int i_S	// height of the column
		);

		// Description:	Perform the 1D forward S+P transform on part of a given line
		//				using the B predictor.
		// Returns:		nothing.
		void SptB1DH_Fwd(
			const unsigned int i_I, // line to process
			const unsigned int i_S	// width of the line
		);

		// Description:	Perform the 1D forward S+P transform on part of a given column
		//				using the B predictor.
		// Returns:		Nothing
		void SptB1DV_Fwd(
			const unsigned int i_I, // column to process
			const unsigned int i_S	// height of the column
		);

		// Description:	Perform the 1D inverse S+P transform on part of a given line
		//				using the B predictor.
		// Returns:		Nothing.
		void SptB1DH_Inv(
			const unsigned int i_I, // line to process
			const unsigned int i_S	// width of the line
		);

		// Description:	Perform the 1D inverse S+P transform on part of a given column
		//				using the B predictor.
		// Returns:		Nothing.
		void SptB1DV_Inv(
			const unsigned int i_I, // column to process
			const unsigned int i_S	// height of the column
		);

		// Description:	Perform the 1D forward S+P transform on part of a given line
		//				using the C predictor.
		// Returns:		Nothing.
		void SptC1DH_Fwd(
			const unsigned int i_I, // line to process
			const unsigned int i_S	// width of the line
		);

		// Description:	Perform the 1D forward S+P transform on part of a given column
		//				using the C predictor.
		// Returns:		Nothing.
		void SptC1DV_Fwd(
			const unsigned int i_I, // column to process
			const unsigned int i_S	// height of the column
		);

		// Description:	Perform the 1D inverse S+P transform on part of a given line
		//				using the C predictor.
		// Returns:		Nothing.
		void SptC1DH_Inv(
			const unsigned int i_I, // line to process
			const unsigned int i_S	// width of the line
		);

		// Description:	Perform the 1D inverse S+P transform on part of a given column
		//				using the C predictor.
		// Returns:		Nothing.
		void SptC1DV_Inv(
			const unsigned int i_I, // column to process
			const unsigned int i_S	// height of the column
		);

		// Description:	Perform the 2D S transform on a given top left part of the block.
		// Returns:		Nothing.
		void St2D(
			const bool i_Forward,	// true if forward transform, false otherwise
			const unsigned int i_W, // width of the block part
			const unsigned int i_H	// height of the block part
		)
		{
			COMP_TRYTHIS_SPEED
			Assert(i_W % 2 == 0, Util::CParamException());
			Assert(i_H % 2 == 0, Util::CParamException());
			if (i_Forward)
			{
				unsigned int i;
				for (i = 0; i < i_H; i++)
					St1DH_Fwd(i, i_W);
				for (i = 0; i < i_W; i++)
					St1DV_Fwd(i, i_H);
			}
			else
			{
				unsigned int i;
				for (i = 0; i < i_W; i++)
					St1DV_Inv(i, i_H);
				for (i = 0; i < i_H; i++)
					St1DH_Inv(i, i_W);
			}
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Perform the 2D S transform on a given top left part of the block
		//				using the A predictor.
		// Returns:		Nothing.
		void SptA2D(
			const bool i_Forward,	// true if forward transform, false otherwise
			const unsigned int i_W, // width of the block part
			const unsigned int i_H	// height of the block part
		)
		{
			COMP_TRYTHIS_SPEED
			Assert(i_W % 2 == 0, Util::CParamException());
			Assert(i_H % 2 == 0, Util::CParamException());
			if (i_Forward)
			{
				unsigned int i;
				for (i = 0; i < i_H; i++)
					SptA1DH_Fwd(i, i_W);
				for (i = 0; i < i_W; i++)
					SptA1DV_Fwd(i, i_H);
			}
			else
			{
				unsigned int i;
				for (i = 0; i < i_W; i++)
					SptA1DV_Inv(i, i_H);
				for (i = 0; i < i_H; i++)
					SptA1DH_Inv(i, i_W);
			}
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Perform the 2D S transform on a given top left part of the block
		//				using the B predictor.
		// Returns:		Nothing.
		void SptB2D(
			const bool i_Forward,	// true if forward transform, false otherwise
			const unsigned int i_W, // width of the block part
			const unsigned int i_H	// height of the block part
		)
		{
			COMP_TRYTHIS_SPEED
			Assert(i_W % 2 == 0, Util::CParamException());
			Assert(i_H % 2 == 0, Util::CParamException());
			if (i_Forward)
			{
				unsigned int i;
				for (i = 0; i < i_H; i++)
					SptB1DH_Fwd(i, i_W);
				for (i = 0; i < i_W; i++)
					SptB1DV_Fwd(i, i_H);
			}
			else
			{
				unsigned int i;
				for (i = 0; i < i_W; i++)
					SptB1DV_Inv(i, i_H);
				for (i = 0; i < i_H; i++)
					SptB1DH_Inv(i, i_W);
			}
			COMP_CATCHTHIS_SPEED
		}

		// Description:	Perform the 2D S transform on a given top left part of the block
		//				using the C predictor.
		// Returns:		Nothing.
		void SptC2D(
			const bool i_Forward,	// true if forward transform, false otherwise
			const unsigned int i_W, // width of the block part
			const unsigned int i_H	// height of the block part
		)
		{
			COMP_TRYTHIS_SPEED
			Assert(i_W % 2 == 0, Util::CParamException());
			Assert(i_H % 2 == 0, Util::CParamException());
			if (i_Forward)
			{
				unsigned int i;
				for (i = 0; i < i_H; i++)
				{
					St1DH_Fwd(i, i_W);
					SptC1DH_Fwd(i, i_W);
				}
				for (i = 0; i < i_W; i++)
				{
					St1DV_Fwd(i, i_H);
					SptC1DV_Fwd(i, i_H);
				}
			}
			else
			{
				unsigned int i;
				for (i = 0; i < i_W; i++)
				{
					SptC1DV_Inv(i, i_H);
					St1DV_Inv(i, i_H);
				}
				for (i = 0; i < i_H; i++)
				{
					SptC1DH_Inv(i, i_W);
					St1DH_Inv(i, i_W);
				}
			}
			COMP_CATCHTHIS_SPEED
		}

	public:
		// INTERFACES:

		// Description:	Modify the block dimensions.
		// Returns:		Nothing.
		void Resize(
			const unsigned int i_W, // new width
			const unsigned int i_H	// new height
		);

		// Description : default constructor
		// Return : nothing
		CWBlock()
			: m_W(0), m_H(0), m_Size(0)
		{
		}

		// Description:	Constructor with user-defined block dimension.
		// Returns:		Nothing.
		CWBlock(
			const unsigned int i_W, // width of the block
			const unsigned int i_H	// height of the block
			)
			: m_W(0), m_H(0), m_Size(0)
		{
			COMP_TRYTHIS
			Resize(i_W, i_H);
			COMP_CATCHTHIS
		}

		// Description:	Gets the block width.
		// Returns:		The width.
		unsigned int GetW()
			const
		{
			return m_W;
		}

		// Description:	Gets the block height.
		// Returns:		The height.
		unsigned int GetH()
			const
		{
			return m_H;
		}

		// Description:	Gets the block size.
		// Returns:		The size.
		unsigned long GetSize()
			const
		{
			return m_Size;
		}

		// Description:	Gets the base address of the elements buffer.
		// Returns:		The base address of the elements buffer.
		int *GetBuffer()
		{
			return &m_Buffer[0];
		}

		// Description:	Gets the base address of the lines pointer array.
		// Returns:		The base address of the lines pointer array.
		int **GetData()
		{
			return &m_Data[0];
		}

		// Description:	Sets all block elements to zero.
		// Returns:		Nothing.
		void Zero()
		{
			COMP_TRYTHIS
			if (m_Size)
				m_Buffer.assign(m_Size, 0);
			COMP_CATCHTHIS
		}

		// Description:	Fill the block with part of the data of a CImage object.
		//				Eventually padding is performed if not all block elements are filled.
		// Returns:		Nothing.
		void GetAndPad(
			CImage &i_Img,			// source CImage object
			const unsigned int i_X, // top left corner column index
			const unsigned int i_Y, // top left corner line index
			const unsigned int i_W, // width of the part
			const unsigned int i_H	// height of the part
		);

		// Description:	Replace part of the data of a CImage object with the block data.
		// Returns:		Nothing.
		void Put(
			CImage &O_Img,			// destination CImage object
			const unsigned int i_X, // top left corner column index
			const unsigned int i_Y, // top left corner line index
			const unsigned int i_W, // width of the part
			const unsigned int i_H	// height of the part
		);

		// Description:	Perform i_NbIte iterations of the S transform on the block data.
		// Returns:		Nothing.
		void IterateSt(
			const bool i_Forward,	   // true if forward transform, false otherwise
			const unsigned int i_NbIte // number of iterations
		);

		// Description:	Perform i_NbIte iterations of the S+P transform on the block data
		//				using the A predictor.
		// Returns:		Nothing.
		void IterateSptA(
			const bool i_Forward,	   // true if forward transform, false otherwise
			const unsigned int i_NbIte // number of iterations
		);

		// Description:	Perform i_NbIte iterations of the S+P transform on the block data
		//				using the B predictor.
		// Returns:		Nothing.
		void IterateSptB(
			const bool i_Forward,	   // true if forward transform, false otherwise
			const unsigned int i_NbIte // number of iterations
		);

		// Description:	Perform i_NbIte iterations of the S+P transform on the block data
		//				using the C predictor.
		// Returns:		Nothing.
		void IterateSptC(
			const bool i_Forward,	   // true if forward transform, false otherwise
			const unsigned int i_NbIte // number of iterations
		);

		// Description:	Gets the maximum coefficient in the block.
		// Returns:		The maximum coefficient.
		int GetMaxCoef() const;

		// Description:	Gets the maximum coefficient in the specified WT quadrant of the block.
		// Returns:		The maximum coefficient.
		int GetQuadrantMaxCoef(
			const unsigned int i_X, // top left corner column index
			const unsigned int i_Y, // top left corner line index
			const unsigned int i_W, // width of the quadrant
			const unsigned int i_H	// height of the quadrant
		) const;
	};

} // end namespace

#endif
