#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"
#include "common/resizeable_buffer.h"

namespace fengyun3
{
    namespace mwts3
    {
        class MWTS3Reader
        {
        private:
            ResizeableBuffer<unsigned short> channels[18];
            time_t lastTime;
            unsigned short lineBuf[1000];

        public:
            MWTS3Reader();
            ~MWTS3Reader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getChannel(int channel);
        };
    }
}