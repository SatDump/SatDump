#pragma once

#include "common/ccsds/ccsds.h"
#include "image/image.h"
#include "xrit/xrit_file.h"
#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace goes
{
    namespace hrit
    {
        enum lrit_image_status
        {
            RECEIVING,
            SAVING,
            IDLE
        };
    } // namespace hrit
} // namespace goes