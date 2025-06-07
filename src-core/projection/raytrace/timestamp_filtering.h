#pragma once

#include "nlohmann/json.hpp"
#include <vector>

namespace satdump
{
    namespace timestamp_filtering
    {
        std::vector<double> filter_timestamps_width_cfg(std::vector<double> timestamps, nlohmann::json timestamps_filter);
    }
} // namespace satdump