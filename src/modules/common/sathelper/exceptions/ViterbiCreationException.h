/*
 * ViterbiCreationException.h
 *
 *  Created on: 04/11/2016
 *      Author: Lucas Teske
 */

#pragma once

#include "SatHelperException.h"

namespace sathelper
{
	class ViterbiCreationException : public SatHelperException
	{
	public:
		ViterbiCreationException() : SatHelperException("Failed to create Viterbi") {}
	};
} // namespace sathelper