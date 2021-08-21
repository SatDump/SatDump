#pragma once

#include "common/ccsds/ccsds.h"

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace metop
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
            void work(ccsds::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    } // namespace amsu
} // namespace metop