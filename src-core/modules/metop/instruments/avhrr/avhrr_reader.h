#pragma once

#include "common/ccsds/ccsds.h"

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace metop
{
    namespace avhrr
    {
        class AVHRRReader
        {
        private:
            unsigned short *channels[5];
            uint16_t avhrrBuffer[12944];

        public:
            AVHRRReader();
            ~AVHRRReader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    } // namespace avhrr
} // namespace metop