#pragma once

#include <string>
#include "dll_export.h"

namespace satdump
{
    SATDUMP_DLL extern std::string user_path;
    void initSatdump();
}