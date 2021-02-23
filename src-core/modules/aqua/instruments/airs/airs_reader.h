#pragma once

#include "modules/common/ccsds/ccsds_1_0_1024/ccsds.h"

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace aqua
{
    namespace airs
    {
        class AIRSReader
        {
        private:
            unsigned short *channels[2666];
            unsigned short *hd_channels[4];

        public:
            AIRSReader();
            ~AIRSReader();
            int lines;
            void work(ccsds::ccsds_1_0_1024::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getChannel(int channel);
            cimg_library::CImg<unsigned short> getHDChannel(int channel);
        };
    } // namespace airs
} // namespace aqua