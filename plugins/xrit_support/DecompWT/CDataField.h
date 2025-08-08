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

#ifndef CDataField_included
#define CDataField_included

/*******************************************************************************

TYPE:
Concrete classes plus some overloaded operators.
					
PURPOSE:
Provides special-purpose containers for the data fields of LRIT/HRIT files.

FUNCTION:
The Util::CDataField class and its inheritants handle dynamically allocated memory
and also provide dimension information on the data held in the heap memory.
This is convenient for passing, for example, image data to processing functions.

INTERFACES:
See 'INTERFACES:' in the module declaration below.

RESOURCES:	
None.

REFERENCES:
None.

PROCESSING:
Member functions of the CDataField class:
- CDataField(): Several constructors are provided (see below for the details of
  the input arguments.
- ~CDataField() is the desctructor. IF no further objects refer to the same data,
  these are deleted from the heap.
- operator=() is the assignment operator.
- Clone() produces a copy of the object.
- get() returns the start address of the allocated memory.
- operator->() returns the start address of the allocated memory.
- Size() returns the size of the data field.
- Resize() returns a data field with modified size. Contents are copied and
  truncated (if size decreases) or zero-padded (if size increases).
- operator>>() loads the contents of the dynamically allocated memory from an
  input stream.
- operator<<() writes the contents of the dynamically allocated memory to an
  output stream.
The classes CDataFieldUncompressedImage and CDataFieldCompressedImage inherit
from CDataField. They are provided with additional specific dimensioning information.

DATA:
See 'DATA:' in the class header below.

LOGIC:		
-

*******************************************************************************/

#include <iostream>
#include <string.h>

#include "SmartPtr.h"	   // Util
#include "ErrorHandling.h" // Util
#include "Types.h"		   // Util

namespace Util
{

	/**
 * Container for LRIT/HRIT file data field.
 **/
	class CDataField : public Util::CSmartPtr<unsigned char>
	{
	protected:
		unsigned __int64 m_Size;	  // Length of used      data field [bits].
		unsigned __int64 m_Allocated; // Length of allocated data field [bits].

	public:
		/**	
 *Constructor.
 **/
		CDataField(void *i_Ptr,					 // Address of an allocated memory area.
				   const unsigned __int64 i_Size // Size [bits] of this area.
				   )
			: Util::CSmartPtr<unsigned char>(static_cast<unsigned char *>(i_Ptr)), m_Size(i_Size), m_Allocated(i_Size)
		{
		}

		/**	
 * Copy constructor.
 **/
		CDataField(
			const CDataField &i_DataField // Reference data field.
			)
			: Util::CSmartPtr<unsigned char>(i_DataField), m_Size(i_DataField.m_Size), m_Allocated(i_DataField.m_Allocated)
		{
		}

		/**
  * Constructor. Copies a string into a CDataField.
  **/
		explicit CDataField(
			const std::string &i_String // Input string.
			) : Util::CSmartPtr<unsigned char>(i_String.size() ? new unsigned char[i_String.size()] : NULL),
				m_Size(i_String.size() * 8),
				m_Allocated(i_String.size() * 8)
		{
			try
			{
				AssertNamed(m_Size == 0 || Data() != NULL, "Memory allocation failed!");

				for (unsigned int i = 0; i < i_String.size(); ++i)
					Data()[i] = i_String[i];
			}
			catch (...)
			{
				LOGCATCHANDTHROW;
			}
		}

		/** 
  * Constructor which produces a new data field of the specified size.
  * This is also the default constructor which creates a NULL CSmartPtr.
  **/
		CDataField(const unsigned __int64 i_Size = 0, // Size [bits] of new data field.
				   const bool i_Initialise = true	  // true : Initialise data field with 0.
													  // false: Do not initialise.
				   )
			: Util::CSmartPtr<unsigned char>(i_Size ? new unsigned char[(size_t)((i_Size + 7) / 8)] : NULL), m_Size(i_Size), m_Allocated(i_Size)
		{
			try
			{
				AssertNamed(m_Size == 0 || Data() != NULL, "Memory allocation failed!");

				if (i_Initialise && (Data() != NULL))
					memset(Data(), 0, (size_t)((m_Size + 7) / 8));
			}
			catch (...)
			{
				LOGCATCHANDTHROW;
			}
		}

		/**
  * Destructor.
  **/
		virtual ~CDataField() {}

		/** 
  * The assignment operator behaves like that of auto_ptr.
  **/
		CDataField &operator=(
			const CDataField &i_DataField // Reference data field.
		)
		{
			try
			{
				static_cast<Util::CSmartPtr<unsigned char> *>(this)->operator=(i_DataField);
				m_Size = i_DataField.m_Size;
				m_Allocated = i_DataField.m_Allocated;
				return *this;
			}
			catch (...)
			{
				LOGCATCHANDTHROW;
			}
		}

		/** 
  *Produces a copy of the object.
  * �@return A copy of the object.
  **/
		CDataField Clone()
			const
		{
			try
			{
				Util::CDataField dataField(m_Size, false);
				memcpy(dataField, (const unsigned char *)(*this), (unsigned int)((m_Size + 7) / 8));
				return dataField;
			}
			catch (...)
			{
				LOGCATCHANDTHROW;
			}
		}

		/**
  * Returns the size of the data field.
  * @returns Size [bits] of the data field.
  **/
		unsigned __int64 Size() const { return m_Size; }

		/** 
  * Returns a data field with modified size. Contents are copied and
  * truncated (if size decreases) or zero-padded (if size increases).
  **/
		virtual CDataField Resize(
			const unsigned __int64 i_Size // Size [bits] of the new data field.
		)
		{
			try
			{
				// New size still fits in allocated memory.
				if (i_Size <= m_Allocated)
				{
					m_Size = i_Size;
					return *this;
				}

				// New size exceeds allocated memory.
				CDataField resized(i_Size, false);
				unsigned __int64 i;
				for (i = 0; (i < (i_Size + 7) / 8) && (i < (m_Size + 7) / 8); ++i)
					((unsigned char *)resized)[i] = (*this)[i];

				for (i = (m_Size + 7) / 8;
					 i < (i_Size + 7) / 8; ++i)
					((unsigned char *)resized)[i] = 0;

				*this = resized;
				return resized;
			}
			catch (...)
			{
				LOGCATCHANDTHROW;
			}
		}

		/**
  *Returns the start address of the allocated memory.
  *@returns:The start address of the allocated memory.
  **/
		unsigned char *Data() { return this->operator->(); }
		const unsigned char *Data() const { return this->operator->(); }

		/** 
  * Loads contents of dynamically allocated memory from an input stream.
  * @returns:	Input stream.
  **/
		friend std::istream &operator>>(
			std::istream &, // Input stream.
			CDataField &	// Data field to be written.
		);

		/** 
  * Writes contents of dynamically allocated memory to an output stream.
  * @returns: Output stream.
  **/
		friend std::ostream &operator<<(
			std::ostream &,	   // Output stream.
			const CDataField & // Data field to be loaded.
		);

		/**
  * Returns the start address of the allocated memory.
  * Only for compatibility with previous versions. Better use Data().
  * @returns: The start address of the allocated memory.
  **/
		unsigned char *get() { return this->operator->(); }
		const unsigned char *get() const { return this->operator->(); }

		/** 
  * Only for compatibility with previous versions. Better use Size().
  * @returns: Returns the size of the data field.
  **/
		unsigned __int64 GetLength() const { return Size(); }

		/** 
  * Modifies the size of the data field.
  * Only for compatibility with previous versions. Better use Resize().
  **/
		virtual void SetLength(
			const unsigned __int64 i_Length // New size [bits] of the data field.
		)
		{
			*this = Resize(i_Length);
		}

		/** 
  * Copies an array of bytes from the data field into the specified array.
  * Checks that no out-of-border read access is attempted to the
  * data field.
  **/
		void Read(
			const int i_Offset,	   // Offset [bytes] from start of data field.
			unsigned char *o_Data, // Start of output byte array.
			const int i_Length	   // Length of byte array to be copied.
		)
			const
		{
			AssertNamed(o_Data != NULL &&
							i_Offset >= 0 &&
							(unsigned long long)i_Offset + (unsigned long long)i_Length <= (m_Size + 7) / 8,
						"Out-of-border read access to data field attempted.");

			memcpy(o_Data, Data() + i_Offset, i_Length);
		}

		/** 
 * Copies a byte array to the specified position in the data field.
 * Checks that no out-of-border write access is attempted to the
 * data field.
 **/
		void Write(
			const int i_Offset,			 // Offset [bytes] from start of data field.
			const unsigned char *i_Data, // Start of byte array to be copied.
			const int i_Length			 // Length of byte array to be copied.
		)
		{
			AssertNamed(i_Data != NULL &&
							i_Offset >= 0 &&
							(unsigned long long)i_Offset + (unsigned long long)i_Length <= (m_Size + 7) / 8,
						"Out-of-border write access to data field attempted.");

			memcpy(Data() + i_Offset, i_Data, i_Length);
		}

		/**
  * Sets all bytes in the specified section of the data field to the
  * given value.
  * Checks that no out-of-border write access is attempted to the
  * data field.
  **/
		void Set(
			const int i_Offset,			// Offset [bytes] from start of data field.
			const int i_Length,			// Length of byte array to be overwritten.
			const unsigned char i_Value // New byte value.
		)
		{
			AssertNamed(i_Offset >= 0 &&
							(unsigned long long)i_Offset + (unsigned long long)i_Length <= (m_Size + 7) / 8,
						"Out-of-border write access to data field attempted.");

			memset(Data() + i_Offset, i_Value, i_Length);
		}
	};

	// Container for LRIT/HRIT file data field (specific for non-compressed image data).
	// The size of the dynamic memory area can be derived from the NC, NL, and NR parameters.
	class CDataFieldUncompressedImage : public Util::CDataField
	{

	protected:
		// DATA:

		unsigned char m_NB;	 // Number of bits per pixel (valid lsbits).
		unsigned short m_NC; // Number of columns.
		unsigned short m_NL; // Number of lines.
		unsigned char m_NR;	 // Number of bits per pixel (representation).

	public:
		// Description:	Prevents use of CDataField::resize().
		// Returns:		Nothing.
		CDataField Resize(
			const unsigned __int64 /*i_Size*/ // New size [bits] of data field.
		)
			const
		{
			return *this;
		}

	private:
    	using Util::CDataField::Resize;

	public:
		// INTERFACES:

		// Description:	Constructor taking as input an address of an allocated memory area
		//				for image data, and the dimensions of this area.
		// Returns:		Nothing.
		CDataFieldUncompressedImage(
			void *i_Ptr,			   // Allocated memory area.
			const unsigned char i_NB,  // Number of pits per pixel (valid lsbits).
			const unsigned short i_NC, // Number of image columns.
			const unsigned short i_NL, // Number of image lines.
			const unsigned char i_NR   // Number of bits per pixel (representation).
			)
			: CDataField(i_Ptr, (unsigned __int64)i_NC *
									(unsigned __int64)i_NL *
									(unsigned __int64)i_NR),
			  m_NB(i_NB), m_NC(i_NC), m_NL(i_NL), m_NR(i_NR)
		{
			PRECONDITION(m_NB <= m_NR);
		}

		// Description:	Constructor taking as input a base CDataField and the dimensions
		//				of the contained image data.
		// Returns:		Nothing.
		CDataFieldUncompressedImage(
			const CDataField &i_DataField, // Reference data field.
			const unsigned char i_NB,	   // Number of pits per pixel (valid lsbits).
			const unsigned short i_NC,	   // Number of image columns.
			const unsigned short i_NL,	   // Number of image lines.
			const unsigned char i_NR	   // Number of bits per pixel (representation).
			)
			: CDataField(i_DataField), m_NB(i_NB), m_NC(i_NC), m_NL(i_NL), m_NR(i_NR)
		{
			PRECONDITION(m_NB <= m_NR);
			PRECONDITION(m_NC * m_NL * m_NR == m_Size);
		}

		// Description:	Copy constructor.
		// Returns:		Nothing.
		CDataFieldUncompressedImage(
			const CDataFieldUncompressedImage &i_DataFieldUI // Reference data field.
			)
			: CDataField(i_DataFieldUI), m_NB(i_DataFieldUI.m_NB), m_NC(i_DataFieldUI.m_NC), m_NL(i_DataFieldUI.m_NL), m_NR(i_DataFieldUI.m_NR)
		{
		}

		// Description:	Constructor which produces a new data field for image data
		//				of the specified dimensions.
		// Returns:		Nothing.
		CDataFieldUncompressedImage(
			const unsigned char i_NB = 1,  // Number of pits per pixel (valid lsbits).
			const unsigned short i_NC = 0, // Number of image columns.
			const unsigned short i_NL = 0, // Number of image lines.
			const unsigned char i_NR = 1,  // Number of bits per pixel (representation).
			const bool i_Initialize = true
			// true : Initialize data field with 0.
			// false: Do not initialize.
			)
			: CDataField((unsigned __int64)i_NC *
							 (unsigned __int64)i_NL *
							 (unsigned __int64)i_NR,
						 i_Initialize),
			  m_NB(i_NB), m_NC(i_NC), m_NL(i_NL), m_NR(i_NR)
		{
			PRECONDITION(m_NB <= m_NR);
		}

		// Description:	Assignment operator.
		// Returns:		Nothing.
		void operator=(
			const CDataFieldUncompressedImage &i_From // Reference data field.
		)
		{
			try
			{
				static_cast<CDataField *>(this)->operator=(i_From);
				this->m_NB = i_From.m_NB;
				this->m_NC = i_From.m_NC;
				this->m_NL = i_From.m_NL;
				this->m_NR = i_From.m_NR;
			}
			catch (...)
			{
				LOGCATCHANDTHROW;
			}
		}

		// Description:	Produces a copy of the object.
		// Returns:		A copy of the object.
		CDataFieldUncompressedImage Clone()
			const
		{
			try
			{
				Util::CDataFieldUncompressedImage dataField(m_NB, m_NC, m_NL, m_NR, false);
				memcpy(dataField, (const unsigned char *)(*this), (unsigned int)((m_Size + 7) / 8));
				return dataField;
			}
			catch (...)
			{
				LOGCATCHANDTHROW;
			}
		}

		// Description: Provide read access to member variables.
		// Returns:		GetNB(): The number of bits per pixel (valid lsbits).
		//				GetNC(): The number of image columns.
		//				GetNL(): The number of image lines.
		//				GetNR(): The number of bits per pixel (representation).
		unsigned char GetNB() const { return m_NB; }
		unsigned short GetNC() const { return m_NC; }
		unsigned short GetNL() const { return m_NL; }
		unsigned char GetNR() const { return m_NR; }
	};

	// Container for LRIT/HRIT file data field (specific for compressed image data).
	// The size of the dynamic memory area can NOT be derived from the NC, NL, and NB parameters.
	class CDataFieldCompressedImage : public Util::CDataField
	{

	protected:
		// DATA:

		unsigned char m_NB;	 // Number of bits per pixel (valid lsbits).
		unsigned short m_NC; // Number of columns.
		unsigned short m_NL; // Number of lines.

	public:
		// INTERFACES:

		// Description: Constructor taking as input an address of an allocated memory area
		//				for image data, and the dimensions of this area.
		// Returns:		Nothing.
		CDataFieldCompressedImage(
			void *i_Ptr,				   // Address of allocated memory.
			const unsigned __int64 i_Size, // Size [bits] of data field.
			const unsigned char i_NB,	   // Number of bits per pixel (valid lsbits).
			const unsigned short i_NC,	   // Number of columns.
			const unsigned short i_NL	   // Number of lines.
			)
			: CDataField(i_Ptr, i_Size), m_NB(i_NB), m_NC(i_NC), m_NL(i_NL)
		{
		}

		// Description: Constructor taking a base CDataField and the dimensions
		//				of the image data contained.
		// Returns:		Nothing.
		CDataFieldCompressedImage(
			const CDataField &i_DataField, // Reference data field.
			const unsigned char i_NB,	   // Number of bits per pixel (valid lsbits).
			const unsigned short i_NC,	   // Number of columns.
			const unsigned short i_NL	   // Number of lines.
			)
			: CDataField(i_DataField), m_NB(i_NB), m_NC(i_NC), m_NL(i_NL)
		{
		}

		// Description:	Copy constructor.
		// Returns:		Nothing.
		CDataFieldCompressedImage(
			const CDataFieldCompressedImage &i_DataField // Reference data field.
			)
			: CDataField(i_DataField), m_NB(i_DataField.GetNB()), m_NC(i_DataField.GetNC()), m_NL(i_DataField.GetNL())
		{
		}

		// Description:	Constructor. Takes an uncompressed image as input argument.
		// Returns:		Nothing.
		explicit CDataFieldCompressedImage(
			const CDataFieldUncompressedImage &i_DataField // Reference data field.
			)
			: CDataField(i_DataField), m_NB(i_DataField.GetNR()), m_NC(i_DataField.GetNC()), m_NL(i_DataField.GetNL())
		{
		}

		// Description:	Constructor which produces a new data field for image data
		//				of the specified dimensions.
		//				This is also the default constructor which creates a NULL auto_ptr.
		// Returns:		Nothing.
		CDataFieldCompressedImage(
			const unsigned __int64 i_Size = 0, // Size [bits] of data field.
			const unsigned char i_NB = 0,	   // Number of bits per pixel (valid lsbits).
			const unsigned short i_NC = 0,	   // Number of columns.
			const unsigned short i_NL = 0,	   // Number of lines.
			const bool i_Initialise = true
			// true : Initialize data field with 0.
			// false: Do not initialize.
			)
			: CDataField(i_Size, i_Initialise), m_NB(i_NB), m_NC(i_NC), m_NL(i_NL)
		{
		}

		// Description:	Assignment operator.
		// Returns:		Nothing.
		void operator=(
			const CDataFieldCompressedImage &i_From // Reference data field.
		)
		{
			try
			{
				static_cast<CDataField *>(this)->operator=(i_From);
				this->m_NB = i_From.m_NB;
				this->m_NC = i_From.m_NC;
				this->m_NL = i_From.m_NL;
			}
			catch (...)
			{
				LOGCATCHANDTHROW;
			}
		}

		// Description:	Produces a copy of the object.
		// Returns:		A copy of the object.
		CDataFieldCompressedImage Clone()
			const
		{
			try
			{
				Util::CDataFieldCompressedImage dataField(m_Size, m_NB, m_NC, m_NL, false);
				memcpy(dataField, (const unsigned char *)(*this), (unsigned int)((m_Size + 7) / 8));
				return dataField;
			}
			catch (...)
			{
				LOGCATCHANDTHROW;
			}
		}

		// Description: Provide read access to member variables.
		// Returns:		GetNB(): The number of bits per pixel (valid lsbits).
		//				GetNC(): The number of image columns.
		//				GetNL(): The number of image lines.
		unsigned char GetNB() const { return m_NB; }
		unsigned short GetNC() const { return m_NC; }
		unsigned short GetNL() const { return m_NL; }
	};

	inline std::istream &operator>>(
		std::istream &i_Stream,
		CDataField &t_DataField)
	{
		try
		{
			i_Stream.read((char *)(unsigned char *)t_DataField,
						  (unsigned int)((t_DataField.Size() + 7) / 8));
			AssertCLib(i_Stream.fail() == false);
			return i_Stream;
		}
		catch (...)
		{
			LOGCATCHANDTHROW;
		}
	}

	inline std::ostream &operator<<(
		std::ostream &i_Stream,
		const CDataField &i_DataField)
	{
		try
		{
			i_Stream.write((const char *)(const unsigned char *)i_DataField,
						   (unsigned int)((i_DataField.Size() + 7) / 8));
			AssertCLib(i_Stream.good());
			return i_Stream;
		}
		catch (...)
		{
			LOGCATCHANDTHROW;
		}
	}

} // end namespace

#endif
