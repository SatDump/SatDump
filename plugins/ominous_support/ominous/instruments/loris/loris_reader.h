#pragma once

#include "common/ccsds/ccsds.h"
#include "image/image.h"
#include <string>
#include <vector>

namespace ominous
{
    namespace loris
    {
        class LORISReader
        {
        public:
            LORISReader();
            ~LORISReader();

            void work(ccsds::CCSDSPacket &packet);

            std::string directory;

            int images;
        };
    } // namespace loris
} // namespace ominous