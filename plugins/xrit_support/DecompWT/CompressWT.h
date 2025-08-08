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

#ifndef CompressWT_included
#define CompressWT_included

/*******************************************************************************

TYPE:
Concrete classes and free functions.
					
PURPOSE:
Data structure for compression parameters.
Classes for each compression algorithm.
Free functions for decompression.

FUNCTION:
Performs compression and decompression of image-type data according to the JPEG
Said & Pearlman algorithm.

INTERFACES:
See 'INTERFACES:' in the module declaration below.

RESOURCES:	
Heap Memory (>2K).

REFERENCES:
None.

PROCESSING:
Derived from the abstract base class CCompress, the concrete classes
- CCompressWT
provides the algorithm specific parameters and Compress() methods.

- DecompressWT() decompresses Wavelet coded input image data.

For passing image data to and from the routines, Util::CDataField... objects are
used.

DATA:
See 'DATA:' in the class header below.

LOGIC:		
-

*******************************************************************************/

#include <sstream>
#include <string>
#include <vector>

#include "CDataField.h"	   // Util
#include "SmartPtr.h"	   // Util
#include "ErrorHandling.h" // Util
#include "Compress.h"	   // COMP

namespace COMP
{

	// Container class for WT compression/decompression parameters.
	class CWTParams
	{

	public:
		// Enumeration of the block modes.
		enum EWTBlockMode
		{
			e_16x16Block = 1, // 16 x 16 blocks.
			e_32x32Block = 2, // 32 x 32 blocks.
			e_64x64Block = 3, // 64 x 64 blocks.
			e_FullWidth = 4	  // Entire image segment is one block.
		};

		// Enumeration of the wavelet transform prediction modes.
		enum EWTPredictionMode
		{
			e_None = 1,	 // No prediction
			e_PredA = 2, // Predictor A
			e_PredB = 3, // Predictor B
			e_PredC = 4	 // Predictor C
		};

		// DATA:

		unsigned int m_BitsPerPixel; // This parameter set is intended to be
									 // applied to images with the specified
									 // number of bits per pixel.

		unsigned int m_nWTlevels; // Number of wavelet transform levels
								  // range: [3-6]

		EWTPredictionMode m_PredMode; // Specifies the kind of prediction to use
									  // with the wavelet transform

		EWTBlockMode m_BlockMode; // Specifies the sizes of the image blocks
								  // to use  for compression;

		unsigned int m_nLossyBitPlanes; // Number of wavelet transform coefficients
										// bit planes that are not coded / decoded
										// Lossless if m_nLossyBitPlanes = 0
										// Lossy if m_nLossyBitPlanes > 0 [1-15]
										// 1 ->  1 lossy bit plane
										// 4 ->  2 lossy bit planes
										// 9 ->  3 lossy bit planes
										// 15 -> 4 lossy bit planes
										// others -> intermediate levels

		unsigned int m_RestartInterval; // Number of image blocks between subsequent
										// restart markers.

	public:
		// INTERFACES:

		// Description:	Constructor. Initialises all member variables.
		// Returns:		Nothing.
		CWTParams(
			unsigned int i_BitsPerPixel,
			unsigned int i_nWTlevels,
			EWTPredictionMode i_PredMode,
			EWTBlockMode i_BlockMode,
			unsigned int i_nLossyBitPlanes,
			unsigned int i_RestartInterval)
			: m_BitsPerPixel(i_BitsPerPixel), m_nWTlevels(i_nWTlevels), m_PredMode(i_PredMode), m_BlockMode(i_BlockMode), m_nLossyBitPlanes(i_nLossyBitPlanes), m_RestartInterval(i_RestartInterval)
		{
			AssertNamed(m_BitsPerPixel >= 1 && m_BitsPerPixel <= 16, "bitsPerPixel value out of range.");
			AssertNamed(m_nWTlevels >= 3 && m_nWTlevels <= 6, "nWTlevels value out of range.");
			AssertNamed(m_nLossyBitPlanes <= 1, "nLossyBitPlanes value out of range.");
			switch (m_BlockMode)
			{
			case CWTParams::e_16x16Block:
				AssertNamed(m_nWTlevels <= 4,
							"nWTlevels value out of range allowed for selected block size 16x16.");
				break;

			case CWTParams::e_32x32Block:
				AssertNamed(m_nWTlevels <= 5,
							"nWTlevels value out of range allowed for selected block size 32x32.");
				break;

			case CWTParams::e_64x64Block:
				AssertNamed(m_nWTlevels <= 6,
							"nWTlevels value out of range allowed for selected block size 64x64.");
				break;

			default:
				break;
			}
		}

		// Description:	Default constructor. Used during decompression.
		// Returns:		Nothing.
		CWTParams()
		{
		}

		// Description:	Destructor.
		// Returns:		Nothing.
		virtual ~CWTParams()
		{
		}

		// Description:	Retruns the set of compresion parameters in string form.
		// Returns:		Compression parameters in readable form.
		std::string GetTraceString()
			const
		{
			std::ostringstream temp;

			temp << "Bits per Pixel          : " << m_BitsPerPixel
				 << "\nWavelet Transform Levels: " << m_nWTlevels
				 << "\nPrediction Mode         : " << (int)m_PredMode
				 << "\nBlock Mode              : " << (int)m_BlockMode
				 << "\nLossy Bit Planes        : " << m_nLossyBitPlanes
				 << "\nRestart Interval        : " << m_RestartInterval;

			return temp.str();
		}
	};

	// Compress() function for WT compression.
	class CCompressWT : public COMP::CCompress, public COMP::CWTParams
	{

	public:
		// INTERFACES:

		// Description:	Compresses the input image data according to
		//				the specified parameters.
		// Returns:		Compressed data field.
		Util::CDataFieldCompressedImage Compress(
			const Util::CDataFieldUncompressedImage &i_Image // Image to compress.
		)
			const;

		// Description:	Constructor.
		// Returns:		Nothing.
		CCompressWT(
			const CWTParams &i_Params)
			: CCompress(), CWTParams(i_Params)
		{
		}

		// Description:	Default constructor.
		//				Only to be used when objects are received from a network stream.
		// Returns:		Compressed data field.
		CCompressWT()
			: CCompress(), CWTParams()
		{
		}

		// Description:	Tells whether compression is lossless or lossy.
		// Returns:		true : Compression is lossless.
		//				false: Compression is lossy.
		bool IsLossless()
			const
		{
			return (m_nLossyBitPlanes == 0);
		}

		// Description:	Retruns the set of compresion parameters in string form.
		// Returns:		Compression parameters in readable form.
		std::string GetTraceString()
			const
		{
			return CWTParams::GetTraceString();
		}
	};

	// Description:	Decompresses Wavelet coded input image data.
	// Returns:		Nothing.
	void DecompressWT(
		const Util::CDataFieldCompressedImage &
			i_Image,			   // Compressed image data.
		const unsigned char &i_NB, // Desired image representation [bits/pixel].
		Util::CDataFieldUncompressedImage &
			o_ImageDecompressed,		  // Decompressed image data.
		std::vector<short> &o_QualityInfo // Vector with one field per image line.
										  // The function will store there the number
										  // of 'good' pixels, counted from the first
										  // column.
										  // If there are no errors, the values will be
										  // equal to the total number of image columns.
	);

} // end namespace

#endif
