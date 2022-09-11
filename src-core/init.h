#pragma once

#include <string>
#include "dll_export.h"

namespace satdump
{
    SATDUMP_DLL extern std::string user_path;
    SATDUMP_DLL extern std::string tle_file_override;
    void initSatdump();
}