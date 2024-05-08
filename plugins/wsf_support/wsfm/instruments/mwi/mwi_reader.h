#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image2/image.h"

namespace wsfm
{
    namespace mwi
    {
        class MWIReader
        {
        private:
            std::vector<uint16_t> channels[17];
            std::vector<uint8_t> wip_full_pkt;
            uint16_t bufline[12133];

        public:
            MWIReader();
            ~MWIReader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            image2::Image getChannel(int channel);
        };
    } // namespace iasi
} // namespace metop