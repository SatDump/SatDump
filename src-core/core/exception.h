#pragma once

#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>

#define satdump_exception(arg) satdump::satdump_exception_t(arg, __FILE__, __LINE__)

// Length of CMAKE_SOURCE_DIR (including the trailing separator), provided by
// the build system. Default to 0 so the header stays usable if it is missing.
#ifndef SOURCE_PATH_SIZE
#define SOURCE_PATH_SIZE 0
#endif

namespace satdump
{
    namespace exception_detail
    {
        // Strip the build machine's source directory from a __FILE__ path.
        //
        // SOURCE_PATH_SIZE only describes __FILE__ when the compiler embeds
        // absolute paths. Builds passing -ffile-prefix-map (as Debian and other
        // distributions do for reproducible builds) hand us an already relative
        // path such as "./src-core/core/plugin.cpp", where that offset is both
        // meaningless and, for most paths, past the end of the string.
        inline std::string trim_source_path(const char *file)
        {
            if (file == nullptr)
                return std::string();

            const char *p = file;

            if (p[0] == '/')
            {
                // Absolute path: skip the source directory when it is present.
                if (std::strlen(p) > (std::size_t)SOURCE_PATH_SIZE)
                    p += SOURCE_PATH_SIZE;
            }
            else if (p[0] == '.' && p[1] == '/')
            {
                // Already relative: only the leading "./" needs to go.
                p += 2;
            }

            return std::string(p);
        }
    }

    class satdump_exception_t : public std::runtime_error
    {
        std::string msg;

    public:
        satdump_exception_t(const std::string &arg, const char *file, int line) : std::runtime_error(arg)
        {
            msg = arg + " => " + exception_detail::trim_source_path(file) + ":" + std::to_string(line);
        }

        ~satdump_exception_t() throw()
        {
        }

        const char *what() const throw()
        {
            return msg.c_str();
        }
    };
}
