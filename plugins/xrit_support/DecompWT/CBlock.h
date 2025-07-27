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

#ifndef CBlock_included
#define CBlock_included

/*******************************************************************************

TYPE:
Concrete Class

PURPOSE:	
Handle generic 8*8 matrix block

FUNCTION:	
This class provides functions to store and access a generic 8*8 elements matrix
      
INTERFACES:	
See 'INTERFACES' in the module declaration below
      
RESOURCES:	
Heap Memory (>2K).
      
REFERENCE:	
None

PROCESSING:	
class CBlock:
	CBlock () : constructs one block without initializing its elements
	CBlock (T &) contructs one block and initializes all the elements with the specified value
	CBlock (const CBlock<T>&) copy constructor
	operator [] : return the element at the specified linear position
	operator () : return the element at the specified 2D position
	Cget () : return the value of the element at the specified 2D position
	Cset () : set the value of the element at the specified 2D position
	
DATA:
See 'DATA :' in the class header below.			
      
LOGIC:
      
*******************************************************************************/

#include "RMAErrorHandling.h"

namespace COMP
{
	template <class T>
	class CBlock
	{
	protected:
		// DATA :

		T m_data[64]; // contains the 64 data elements

	public:
		// INTERFACES :

		// Description:	default constructor
		// Return:		Nothing
		inline CBlock()
		{
		}

		// Description:	constructor and initialization of all the data elements with the given value
		// Return:		Nothing
		inline CBlock(
			const T &i_val // initialization value
		)
		{
			COMP_TRYTHIS
			for (int i = 0; i < 64; i++)
				m_data[i] = i_val;
			COMP_CATCHTHIS
		}

		// Description:	copy constructor
		// Return:		Nothing
		inline CBlock(
			const CBlock<T> &i_copy // object to be copied
		)
		{
			COMP_TRYTHIS
			for (int i = 0; i < 64; i++)
				m_data[i] = i_copy.m_data[i];
			COMP_CATCHTHIS
		}

		// Description:	destructor
		// Return:		Nothing
		inline ~CBlock()
		{
		}

		// Description:	gets a reference of the specified element
		// Returns:		the reference to the specified element
		inline T &operator[](
			const unsigned int &i_index // 1D index of the element
		)
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(i_index < 64, Util::CParamException());
#endif
			return m_data[i_index];
			COMP_CATCHTHIS_SPEED
		}

		// Description:	gets a reference to the 2D specified element [1D index=8*i_y+i_x]
		// Returns:		the reference to the specified element
		inline T &operator()(
			const unsigned int &i_y, // line index
			const unsigned int &i_x	 // column index
		)
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(i_y < 8 && i_x < 8, Util::CParamException());
#endif
			return m_data[(i_y << 3) + i_x];
			COMP_CATCHTHIS_SPEED
		}

		// Description:	gets the value of the 2D specified element [1D index=8*i_y+i_x]
		// Returns:		the value of the specified element
		inline T Cget(
			const unsigned int &i_y, // line index
			const unsigned int &i_x	 // column index
		)
			const
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(i_y < 8 && i_x < 8, Util::CParamException());
#endif
			return m_data[(i_y << 3) + i_x];
			COMP_CATCHTHIS_SPEED
		}

		// Description:	gets the value of the specified element [1D index=i_k]
		// Returns:		the value of the specified element
		inline T Cget(
			const unsigned int &i_k // linear index
		)
			const
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(i_k < 64, Util::CParamException());
#endif
			return m_data[i_k];
			COMP_CATCHTHIS_SPEED
		}

		// Description:	sets the value of the 2D specified element [1D index=8*i_y+i_x]
		// Returns:		nothing
		inline void Cset(
			const unsigned int &i_y, // line index
			const unsigned int &i_x, // column index
			const T &i_val			 // value
		)
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(i_y < 8 && i_x < 8, Util::CParamException());
#endif
			m_data[(i_y << 3) + i_x] = i_val;
			COMP_CATCHTHIS_SPEED
		}

		// Description:	sets the value of the specified element [1D index=i_k]
		// Returns:		nothing
		inline void Cset(
			const unsigned int &i_k, // linear index
			const T &i_val			 // value
		)
		{
			COMP_TRYTHIS_SPEED
#ifdef _DEBUG
			Assert(i_k < 64, Util::CParamException());
#endif
			m_data[i_k] = i_val;
			COMP_CATCHTHIS_SPEED
		}

		// Description:	gets base address of the elements buffer
		// Returns:		the base address of the elements buffer
		inline T *Get()
		{
			return m_data;
		}
	};

} // end namespace

#endif
