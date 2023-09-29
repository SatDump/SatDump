#pragma once

#include "nlohmann/json.hpp"
#include "common/tracking/tle.h"
#include "common/projection/thinplatespline.h"

namespace satdump
{
    namespace gcp_compute
    {
        /*
        Generic wrapper functions for computing GCPs,
        from a product file or otherwise.

        cfg : Holds the configuration, such as instrument pointing,
        spacing between GCPs and so on.
        tle : The two line element set to compute the sat orbit's
        timestamps : Holds timestamps, used to compute the satellite's
        positions and then GCPs. They order stored in JSON to abstract
        the underlaying data type, which can vary between instruments
        and modes.
        */
        std::vector<satdump::projection::GCP> compute_gcps(nlohmann::ordered_json cfg, int width = -1, int height = -1);
    }
}