#pragma once

#include <ccsds/ccsds.h>
#include <cmath>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace aqua
{
    namespace ceres
    {
        class CERESReader
        {
        private:
            unsigned short *channels[3];

        public:
            CERESReader();
            int lines;
            void work(libccsds::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getImage(int channel);
        };
    } // namespace ceres
} // namespace aqua