#pragma once

#include "common/ccsds/ccsds.h"
#include "image/image.h"

namespace metopsg
{
    namespace mws
    {
        class MWSReader
        {
        public:
            int lines[24];
            std::vector<uint16_t> channels[24];
            std::vector<double> timestamps[24];

        public:
            MWSReader();
            ~MWSReader();
            void work(ccsds::CCSDSPacket &packet);

            void correlate();

            image::Image getChannel(int c);
        };
    } // namespace mws
} // namespace metopsg