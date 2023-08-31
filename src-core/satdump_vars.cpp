#define SATDUMP_DLL_EXPORT 1
#include "satdump_vars.h"

#ifdef __APPLE__
#include <filesystem>
#include <mach-o/dyld.h>
#endif

namespace satdump
{
#if defined (__APPLE__)
        std::string get_macos_exec_dir()
        {
            uint32_t bufsize = PATH_MAX;
            char exec_path[bufsize], real_path[bufsize];
            _NSGetExecutablePath(exec_path, &bufsize);
            realpath(exec_path, real_path);
            std::string ret_val = std::string(real_path);
            return ret_val.substr(0, ret_val.find_last_of("/") + 1);
        }
        std::string init_res_path()
        {
            std::string exec_dir = get_macos_exec_dir();
            if (std::filesystem::exists(exec_dir + "../Resources/") && std::filesystem::is_directory(exec_dir + "../Resources/"))
                return exec_dir + "../Resources/";
            else
                return std::string(RESOURCES_PATH) + "/";
        }
        std::string init_lib_path()
        {
            std::string exec_dir = get_macos_exec_dir();
            if (std::filesystem::exists(exec_dir + "../Resources/") && std::filesystem::is_directory(exec_dir + "../Resources/"))
                return exec_dir + "../Resources/";
            else
                return std::string(LIBRARIES_PATH) + "/";
        }
#elif defined (__ANDROID__)
        std::string init_res_path()
        {
            return "./";
        }
        std::string init_lib_path()
        {
            return "./";
        }
#else
        std::string init_res_path()
        {
            return std::string(RESOURCES_PATH) + "/";
        }
        std::string init_lib_path()
        {
            return std::string(LIBRARIES_PATH) + "/";
        }
#endif
        std::string RESPATH = init_res_path();
        std::string LIBPATH = init_lib_path();
}