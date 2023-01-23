#include "samplerate_to_string.h"
#include "spdlog/fmt/fmt.h"

std::string formatSamplerateToString(uint64_t samplerate)
{
    if (samplerate < 1e3)
        return fmt::format("{:1}", samplerate);
    else if (samplerate < 1e6)
        return fmt::format("{:1.3f}k", samplerate / 1e3);
    else if (samplerate < 1e9)
        return fmt::format("{:1.3f}M", samplerate / 1e6);
    else
        return fmt::format("{:1.3f}");
}
