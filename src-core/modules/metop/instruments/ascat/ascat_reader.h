#pragma once

#include "modules/common/ccsds/ccsds_1_0_1024/ccsds.h"

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace metop
{
    namespace ascat
    {
        class ASCATReader
        {
        private:
            unsigned short *channels[6];

        public:
            ASCATReader();
            ~ASCATReader();
            int lines[6];
            void work(ccsds::ccsds_1_0_1024::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    } // namespace avhrr
} // namespace metop