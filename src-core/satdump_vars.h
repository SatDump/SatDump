#pragma once

#include "dll_export.h"
#include <string>

namespace satdump
{
    SATDUMP_DLL extern const std::string RESOURCES_PATH;
    SATDUMP_DLL extern const std::string LIBRARIES_PATH;
    SATDUMP_DLL extern const std::string SATDUMP_VERSION;
    SATDUMP_DLL extern const std::string SATDUMP_VERSION_TAG;

    SATDUMP_DLL extern std::string RESPATH;
    SATDUMP_DLL extern std::string LIBPATH;

    std::string getSatDumpVersionName();
} // namespace satdump