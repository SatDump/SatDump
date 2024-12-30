#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"
#include "../../aws_crc.h"

namespace aws
{
    namespace mws
    {
        class MWSReader
        {
        private:
            std::vector<uint16_t> channels[19];
            std::vector<uint8_t> wip_full_pkt;
            AWSCRC crc;

        public:
            MWSReader();
            ~MWSReader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            image::Image getChannel(int channel);
        };
    } // namespace iasi
} // namespace metop