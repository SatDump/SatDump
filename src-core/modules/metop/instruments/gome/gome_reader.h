#pragma once

#include "common/ccsds/ccsds.h"

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace metop
{
    namespace gome
    {
        class GOMEReader
        {
        private:
            unsigned short *channels[6144];

        public:
            GOMEReader();
            ~GOMEReader();
            int lines;
            void work(ccsds::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    } // namespace gome
} // namespace metop