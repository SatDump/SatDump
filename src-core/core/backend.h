#pragma once
#include <functional>
#include "dll_export.h"

namespace backend
{
	SATDUMP_DLL extern std::function<void(int, int)> setMousePos;
}