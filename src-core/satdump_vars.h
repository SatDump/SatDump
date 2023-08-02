#pragma once

#include <string>
#include "dll_export.h"

#ifndef RESOURCES_PATH
#define RESOURCES_PATH "./"
#endif

#ifndef LIBRARIES_PATH
#define LIBRARIES_PATH "./"
#endif

#ifndef SATDUMP_VERSION
#define SATDUMP_VERSION "0.0.0-dev"
#endif

namespace satdump
{
   SATDUMP_DLL extern std::string RESPATH;
   SATDUMP_DLL extern std::string LIBPATH;
}