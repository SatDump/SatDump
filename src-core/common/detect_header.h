#pragma once

#include <string>

struct HeaderInfo
{
    bool valid = false;
    uint64_t samplerate = 0;
    std::string type;
};

HeaderInfo try_parse_header(std::string file);