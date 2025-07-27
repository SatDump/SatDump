#pragma once

#include "common/ccsds/ccsds.h"
#include "common/lrit/lrit_file.h"
#include "image/image.h"
#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace elektro
{
    namespace lrit
    {
        struct GOMSxRITProductMeta
        {
            std::string filename;

            int bit_depth = -1;
            int channel = -1;
            std::string satellite_name;
            std::string satellite_short_name;
            time_t scan_time = 0;
            std::shared_ptr<::lrit::ImageNavigationRecord> image_navigation_record;
        };

        enum lrit_image_status
        {
            RECEIVING,
            SAVING,
            IDLE
        };
    } // namespace lrit
} // namespace elektro