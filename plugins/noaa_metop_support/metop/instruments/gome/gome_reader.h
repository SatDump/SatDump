#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"

namespace metop
{
    namespace gome
    {
        class GOMEReader
        {
        private:
            unsigned short *channels[6144];

        public:
            GOMEReader();
            ~GOMEReader();
            int lines;
            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace gome
} // namespace metop