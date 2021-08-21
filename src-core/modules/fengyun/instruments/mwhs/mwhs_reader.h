#pragma once

#include "common/ccsds/ccsds.h"
#include <map>
#include <array>

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace fengyun
{
    namespace mwhs
    {
        class MWHSReader
        {
        private:
            std::map<time_t, std::array<std::array<unsigned short, 98>, 6>> imageData;
            time_t lastTime;
            unsigned short lineBuf[1000];

        public:
            MWHSReader();
            ~MWHSReader();
            int lines;
            void work(ccsds::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    }
}