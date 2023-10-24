#define SATDUMP_DLL_EXPORT 1
#include "satdump_vars.h"

#if defined (__APPLE__)
#include <filesystem>
#include <mach-o/dyld.h>
#include <libgen.h>
#define LIBRARIES_SEARCH_PATH "/../Resources/"
#define RESOURCES_SEARCH_PATH "/../Resources/"
#elif defined (_WIN32)
#include <filesystem>
#include <Windows.h>
#include <shlwapi.h>
#define LIBRARIES_SEARCH_PATH "\\..\\lib\\satdump\\"
#define RESOURCES_SEARCH_PATH "\\..\\share\\satdump\\"
#endif

namespace satdump
{
#if defined (__APPLE__)
        std::string get_search_path(const char *target)
        {
            uint32_t bufsize = PATH_MAX;
            char exec_path[bufsize], ret_val[bufsize], search_dir[bufsize];
            char *exec_dir;
            _NSGetExecutablePath(exec_path, &bufsize);
            exec_dir = dirname(exec_path);
            strcpy(search_dir, exec_dir);
            strcat(search_dir, target);
            realpath(search_dir, ret_val);
            return std::string(ret_val) + "/";
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
#endif

#if defined (__APPLE__) || defined (_WIN32)
        std::string init_res_path()
        {
            std::string search_dir = get_search_path(RESOURCES_SEARCH_PATH);
            if (std::filesystem::exists(search_dir) && std::filesystem::is_directory(search_dir))
                return search_dir;
            else
                return std::string(RESOURCES_PATH);
        }
        std::string init_lib_path()
        {
            std::string search_dir = get_search_path(LIBRARIES_SEARCH_PATH);
            if (std::filesystem::exists(search_dir) && std::filesystem::is_directory(search_dir))
                return search_dir;
            else
                return std::string(LIBRARIES_PATH);
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
            return std::string(RESOURCES_PATH);
        }
        std::string init_lib_path()
        {
            return std::string(LIBRARIES_PATH);
        }
#endif
        std::string RESPATH = init_res_path();
        std::string LIBPATH = init_lib_path();
}