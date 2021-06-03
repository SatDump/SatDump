#pragma once

#include "common/ccsds/ccsds_1_0_1024/ccsds.h"
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
            ~ATMSReader();
            int lines;
            void work(ccsds::ccsds_1_0_1024::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getImage(int channel);
        };
    } // namespace atms
} // namespace jpss