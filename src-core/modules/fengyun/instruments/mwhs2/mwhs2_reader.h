#pragma once

#include "common/ccsds/ccsds_1_0_1024/ccsds.h"
#include <map>
#include <array>

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace fengyun
{
    namespace mwhs2
    {
        class MWHS2Reader
        {
        private:
            std::map<time_t, std::array<std::array<unsigned short, 98>, 15>> imageData;
            time_t lastTime;
            unsigned short lineBuf[1000];

        public:
            MWHS2Reader();
            ~MWHS2Reader();
            int lines;
            void work(ccsds::ccsds_1_0_1024::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    }
}