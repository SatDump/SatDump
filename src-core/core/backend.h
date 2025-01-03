#pragma once
#include <functional>
#include <utility>
#include <stdint.h>
#include "dll_export.h"

namespace backend
{
    SATDUMP_DLL extern float device_scale;

    SATDUMP_DLL extern std::function<void()> rebuildFonts;
    SATDUMP_DLL extern std::function<void(int, int)> setMousePos;
    SATDUMP_DLL extern std::function<std::pair<int, int>()> beginFrame;
    SATDUMP_DLL extern std::function<void()> endFrame;
    SATDUMP_DLL extern std::function<void(uint8_t*, int, int)> setIcon;
}
