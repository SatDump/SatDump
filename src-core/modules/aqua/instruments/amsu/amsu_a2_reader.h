#pragma once

#include "modules/common/ccsds/ccsds_1_0_1024/ccsds.h"

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace aqua
{
    namespace amsu
    {
        class AMSUA2Reader
        {
        private:
            unsigned short *channels[2];
            uint16_t lineBuffer[12944];

        public:
            AMSUA2Reader();
            ~AMSUA2Reader();
            int lines;
            void work(ccsds::ccsds_1_0_1024::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    } // namespace amsu
} // namespace aqua