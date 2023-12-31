#pragma once
#include <functional>
#include <tuple>
#include <stdint.h>
#include "dll_export.h"

namespace backend
{
	SATDUMP_DLL extern std::function<void(int, int)> setMousePos;
	SATDUMP_DLL extern std::function<std::pair<int, int>()> beginFrame;
	SATDUMP_DLL extern std::function<void()> endFrame;
	SATDUMP_DLL extern std::function<void(uint8_t*)> setIcon;
}