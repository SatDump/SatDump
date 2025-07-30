#pragma once

#include "common/ccsds/ccsds.h"
#include "image/image.h"
#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace fy4
{
    namespace lrit
    {
        enum lrit_image_status
        {
            RECEIVING,
            SAVING,
            IDLE
        };
    } // namespace lrit
} // namespace fy4