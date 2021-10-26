#pragma once

#include "common/ccsds/ccsds.h"
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
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
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    }
}