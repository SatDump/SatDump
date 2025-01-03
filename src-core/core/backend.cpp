#define SATDUMP_DLL_EXPORT 1
#include "backend.h"

namespace backend
{
    SATDUMP_DLL float device_scale;

    SATDUMP_DLL std::function<void()> rebuildFonts;
    SATDUMP_DLL std::function<void(int, int)> setMousePos;
    SATDUMP_DLL std::function<std::pair<int, int>()> beginFrame;
    SATDUMP_DLL std::function<void()> endFrame;
    SATDUMP_DLL std::function<void(uint8_t*, int, int)> setIcon;
}
