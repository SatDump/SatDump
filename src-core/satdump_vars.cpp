#define SATDUMP_DLL_EXPORT 1
#include "satdump_vars.h"

namespace satdump
{
    std::string init_res_path()
    {
#ifdef __ANDROID__
        return "./";
#endif

#ifdef _WIN32
        return std::string(RESOURCES_PATH) + "/";
#else
        // const char *val = std::getenv("APPDIR"); // We might be in an AppImage!
        // if (val == nullptr)
        return std::string(RESOURCES_PATH) + "/";
        // else
        //     return std::string(val) + "/usr/share/satdump/";
#endif
    }

    std::string RESPATH = init_res_path();
}