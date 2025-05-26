#pragma once

#include "common/ccsds/ccsds.h"
#include "image/image.h"

namespace fengyun3
{
    namespace mwts3
    {
        class MWTS3Reader
        {
        private:
            std::vector<uint16_t> channels[18];
            unsigned short lineBuf[1000];

        public:
            MWTS3Reader();
            ~MWTS3Reader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            image::Image getChannel(int channel);
        };
    }
}