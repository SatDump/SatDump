#pragma once

#include <string>
#include <cstdint>
#include "nlohmann/json.hpp"

struct HeaderInfo
{
    bool valid = false;
    uint64_t samplerate = 0;
    std::string type;
};

HeaderInfo try_parse_header(std::string file);
void try_get_params_from_input_file(nlohmann::json &parameters, std::string input_file);
