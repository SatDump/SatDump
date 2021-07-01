#pragma once

#include "common/ccsds/ccsds.h"

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace metop
{
    namespace iasi
    {
        class IASIReader
        {
        private:
            unsigned short *channels[8461];

        public:
            IASIReader();
            ~IASIReader();
            int lines;
            void work(ccsds::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    } // namespace iasi
} // namespace metop