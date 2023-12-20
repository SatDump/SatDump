#define SATDUMP_DLL_EXPORT 1
#include "backend.h"

namespace backend
{
	SATDUMP_DLL std::function<void(int, int)> setMousePos;
}