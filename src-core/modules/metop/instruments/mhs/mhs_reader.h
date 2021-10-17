#pragma once

#include "common/ccsds/ccsds.h"

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
            uint16_t mhs_buffer[540];

        public:
            MHSReader();
            ~MHSReader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    } // namespace mhs
} // namespace metop