#pragma once

#include <ccsds/ccsds.h>
#include <cmath>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace jpss
{
    namespace atms
    {
        class ATMSReader
        {
        private:
            int endSequenceCount;
            unsigned short *channels[22];
            bool inScan;

        public:
            ATMSReader();
            int lines;
            void work(libccsds::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getImage(int channel);
        };
    } // namespace atms
} // namespace jpss