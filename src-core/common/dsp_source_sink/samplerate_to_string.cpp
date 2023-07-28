#include "samplerate_to_string.h"
#include <vector>

template <typename... T>
std::string vformat(const char *fmt, T &&...args)
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

std::string formatSamplerateToString(uint64_t samplerate)
{
    if (samplerate < 1e3)
        return vformat("%d", samplerate);
    else if (samplerate < 1e6)
        return vformat("%1.3fk", samplerate / 1e3);
    else if (samplerate < 1e9)
        return vformat("%1.3fM", samplerate / 1e6);
    else
        return vformat("%1.3fG", samplerate / 1e9);
}

std::string formatFrequencyToString(uint64_t samplerate)
{
    if (samplerate < 1e4)
        return vformat("%d", samplerate);
    else if (samplerate < 1e7)
        return vformat("%1.3fk", samplerate / 1e3);
    else if (samplerate < 1e10)
        return vformat("%1.3fM", samplerate / 1e6);
    else
        return vformat("%1.3fG", samplerate / 1e9);
}
