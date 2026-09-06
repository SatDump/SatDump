#pragma once

#include <stdexcept>
#include <string>

#define satdump_exception(arg) satdump::satdump_exception_t(arg, __FILE__, __LINE__)

namespace satdump
{
    class satdump_exception_t : public std::runtime_error
    {
        std::string msg;

    public:
        satdump_exception_t(const std::string &arg, const char *file, int line) : std::runtime_error(arg)
        {
            std::string str(file);
            msg = arg + " => " + str.substr(std::min<int>(str.length(), SOURCE_PATH_SIZE)) + ":" + std::to_string(line);
        }

        ~satdump_exception_t() throw() {}

        const char *what() const throw() { return msg.c_str(); }
    };
} // namespace satdump