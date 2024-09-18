#pragma once
#include <stdint.h>

namespace widgets
{
	bool FrequencyInput(const char *label, uint64_t *frequency_hz, float scale = 0.0f, bool allow_mousewheel = true);
}