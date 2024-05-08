#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image2/image.h"
#include "common/resizeable_buffer.h"

namespace fengyun3
{
    namespace mwts3
    {
        class MWTS3Reader
        {
        private:
            std::vector<uint16_t> channels[18];
            time_t lastTime;
            unsigned short lineBuf[1000];

        public:
            MWTS3Reader();
            ~MWTS3Reader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            image2::Image getChannel(int channel);
        };
    }
}