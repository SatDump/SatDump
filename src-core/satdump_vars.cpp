#define SATDUMP_DLL_EXPORT 1
#include "satdump_vars.h"

#if defined (__APPLE__)
#include <filesystem>
#include <mach-o/dyld.h>
#include <libgen.h>
#elif defined (_WIN32)
#include <filesystem>
#include <Windows.h>
#include <shlwapi.h>
#endif

namespace satdump
{
#if defined (__APPLE__)
        std::string get_search_path(const char *target)
        {
            uint32_t bufsize = PATH_MAX;
            char exec_path[bufsize], ret_val[bufsize];
            _NSGetExecutablePath(exec_path, &bufsize);
            dirname(exec_path);
            strcat(exec_path, target);
            realpath(exec_path, ret_val);
            return std::string(ret_val);
        }
        std::string init_res_path()
        {
            std::string search_dir = get_search_path("/../Resources/");
            if (std::filesystem::exists(search_dir) && std::filesystem::is_directory(search_dir))
                return search_dir;
            else
                return std::string(RESOURCES_PATH) + "/";
        }
        std::string init_lib_path()
        {
            std::string search_dir = get_search_path("/../Resources/");
            if (std::filesystem::exists(search_dir) && std::filesystem::is_directory(search_dir))
                return search_dir;
            else
                return std::string(LIBRARIES_PATH) + "/";
        }
#elif defined (_WIN32)
        std::string get_search_path(const char *target)
        {
            char exe_path[MAX_PATH], ret_val[MAX_PATH];
            GetModuleFileNameA(NULL, exe_path, MAX_PATH);
            PathRemoveFileSpecA(exe_path);
            strcat_s(exe_path, MAX_PATH, target);
            PathCanonicalizeA(ret_val, exe_path);
            return std::string(ret_val);
        }
        std::string init_res_path()
        {
            std::string search_dir = get_search_path("\\..\\share\\satdump\\");
            if (std::filesystem::exists(search_dir) && std::filesystem::is_directory(search_dir))
                return search_dir;
            else
                return std::string(RESOURCES_PATH) + "/";
        }
        std::string init_lib_path()
        {
			std::string search_dir = get_search_path("\\..\\lib\\satdump\\");
            if (std::filesystem::exists(search_dir) && std::filesystem::is_directory(search_dir))
                return search_dir;
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