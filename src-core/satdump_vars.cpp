#define SATDUMP_DLL_EXPORT 1
#include "satdump_vars.h"

#ifdef __APPLE__
#include <filesystem>
#endif

namespace satdump
{
        std::string init_res_path()
        {
#ifdef __ANDROID__
                return "./";
#endif

#if defined(_WIN32)
                return std::string(RESOURCES_PATH) + "/";
#elif defined(__APPLE__)
                if (std::filesystem::exists("../Resources/") && std::filesystem::is_directory("../Resources/"))
					return "../Resources/";
				else
					return std::string(RESOURCES_PATH) + "/";
#else
                // const char *val = std::getenv("APPDIR"); // We might be in an AppImage!
                // if (val == nullptr)
                return std::string(RESOURCES_PATH) + "/";
                // else
                //     return std::string(val) + "/usr/share/satdump/";
#endif
        }

        std::string init_lib_path()
        {
#ifdef __ANDROID__
                return "./";
#endif

#if defined(_WIN32)
                return std::string(LIBRARIES_PATH) + "/";
#elif defined(__APPLE__)
                if (std::filesystem::exists("../Resources/") && std::filesystem::is_directory("../Resources/"))
					return "../Resources/";
				else
					return std::string(LIBRARIES_PATH) + "/";
#else
                // const char *val = std::getenv("APPDIR"); // We might be in an AppImage!
                // if (val == nullptr)
                return std::string(LIBRARIES_PATH) + "/";
                // else
                //     return std::string(val) + "/usr/share/satdump/";
#endif
        }

        std::string RESPATH = init_res_path();
        std::string LIBPATH = init_lib_path();
}