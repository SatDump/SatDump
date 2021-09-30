#pragma once

#include "common/ccsds/ccsds.h"

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
            std::vector<double> timestamps[6];
            void work(ccsds::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    } // namespace avhrr
} // namespace metop