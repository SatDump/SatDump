#pragma once

#include <cstdint>
#include <vector>
#include "nlohmann/json.hpp"

namespace timestamp_filtering
{
    std::vector<double> filter_timestamps_width_cfg(std::vector<double> timestamps, nlohmann::json timestamps_filter);
}