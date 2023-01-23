#pragma once

#include <string>
#include "dll_export.h"

namespace satdump
{
    SATDUMP_DLL extern std::string user_path;
    SATDUMP_DLL extern std::string tle_file_override;
    SATDUMP_DLL extern bool tle_do_update_on_init;
    void initSatdump();
}