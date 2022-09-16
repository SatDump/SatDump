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

#define NOMINMAX

#include "ErrorHandling.h"
#include "CompressT4.h"
#include "T4Coder.h"
#include "T4Decoder.h"

namespace COMP
{

	Util::CDataFieldCompressedImage CCompressT4::Compress(
		const Util::CDataFieldUncompressedImage &i_uncompressedImage)
		const
	{
		COMP_TRYTHIS
		CT4Coder c(i_uncompressedImage);
		c.CodeBuffer();
		return c.GetCompressedImage();
		COMP_CATCHTHIS
	}

	void DecompressT4(const Util::CDataFieldCompressedImage &i_compressedImage,
					  Util::CDataFieldUncompressedImage &o_decompressedImage,
					  std::vector<short> &o_QualityInfo)
	{
		COMP_TRYTHIS
		CT4Decoder d(i_compressedImage); // initialize the beast
										 // and autosize if needed
		d.DecodeBuffer();				 // decode & fill QualityInfo array
		o_decompressedImage = d.GetDecompressedImage();
		o_QualityInfo = d.GetQualityInfo();
		COMP_CATCHTHIS
	}

} // end namespace
