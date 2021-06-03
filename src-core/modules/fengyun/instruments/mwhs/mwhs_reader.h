#pragma once

#include "common/ccsds/ccsds_1_0_1024/ccsds.h"
#include <vector>

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace fengyun
{
    namespace mwhs
    {
        class MWHSImage
        {
        public:
            unsigned short channels[6][99 * 32];
            int mk = -1;
            int lastMkMatch;
        };

        class MWHSReader
        {
        private:
            std::vector<MWHSImage> imageVector;
            uint8_t byteBufShift[3];
            unsigned short lineBuf[1000];

        public:
            MWHSReader();
            ~MWHSReader();
            int lines;
            void work(ccsds::ccsds_1_0_1024::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    }
}