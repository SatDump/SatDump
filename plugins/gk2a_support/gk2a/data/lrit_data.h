#pragma once

#include "common/ccsds/ccsds.h"
#include "common/lrit/lrit_file.h"
#include "image/image.h"
#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace gk2a
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
} // namespace gk2a