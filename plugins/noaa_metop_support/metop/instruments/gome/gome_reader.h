#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"

namespace metop
{
    namespace gome
    {
        class GOMEReader
        {
        public:
            int lines;
            std::vector<uint16_t> channels[6144];

        public:
            GOMEReader();
            ~GOMEReader();
            void work(ccsds::CCSDSPacket &packet);
            std::vector<double> timestamps;
            image::Image getChannel(int channel);
        };
    } // namespace gome
} // namespace metop