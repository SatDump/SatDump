#pragma once

/**
 * @file format.h
 * @brief String formatting utils
 */

#include <sstream>
#include <vector>

namespace satdump
{
    /**
     * @brief Format a number to string with a specific
     * float precision
     *
     * @param a_value the number to format
     * @param n precision to use
     * @return formatted string
     */
    template <typename T>
    std::string to_string_with_precision(const T a_value, const int n = 6)
    {
        std::ostringstream out;
        out.precision(n);
        out << std::fixed << a_value;
        return out.str();
    }

    /**
     * @brief Format a std::string with a standard printf
     * formatting pattern.
     *
     * @param fmt formatting string
     * @param args arguments
     * @return formatted string
     */
    template <typename... T>
    std::string svformat(const char *fmt, T &&...args)
    {
        // Allocate a buffer on the stack that's big enough for us almost
        // all the time.
        size_t size = 1024;
        std::vector<char> buf;
        buf.resize(size);

        // Try to vsnprintf into our buffer.
        size_t needed = snprintf((char *)&buf[0], size, fmt, args...);
        // NB. On Windows, vsnprintf returns -1 if the string didn't fit the
        // buffer.  On Linux & OSX, it returns the length it would have needed.

        if (needed <= size)
        {
            // It fit fine the first time, we're done.
            return std::string(&buf[0]);
        }
        else
        {
            // vsnprintf reported that it wanted to write more characters
            // than we allotted.  So do a malloc of the right size and try again.
            // This doesn't happen very often if we chose our initial size
            // well.
            size = needed;
            buf.resize(size);
            needed = snprintf((char *)&buf[0], size, fmt, args...);
            return std::string(&buf[0]);
        }
    }
} // namespace satdump