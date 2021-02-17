/*
 * WordSizeException.h
 *
 *  Created on: 04/11/2016
 *      Author: Lucas Teske
 */
#pragma once

#include <sstream>
#include "SatHelperException.h"

namespace sathelper
{
	class WordSizeException : public SatHelperException
	{
	public:
		WordSizeException(uint8_t expectedWordSize, uint8_t currentWordSize)
		{
			std::stringstream ss;
			ss << "Tried to add word with size " << expectedWordSize << " but there is already a word with size " << currentWordSize;
			this->msg_ = ss.str();
		}
	};
} // namespace sathelper