#pragma once

#include "common/ccsds/ccsds_1_0_1024/ccsds.h"

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace metop
{
    namespace mhs
    {
        class MHSReader
        {
        private:
            unsigned short *channels[5];
            uint16_t lineBuffer[12944];

        public:
            MHSReader();
            ~MHSReader();
            int lines;
            void work(ccsds::ccsds_1_0_1024::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    } // namespace mhs
} // namespace metop