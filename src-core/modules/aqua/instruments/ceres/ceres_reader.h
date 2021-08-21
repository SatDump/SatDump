#pragma once

#include "common/ccsds/ccsds.h"
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
            ~CERESReader();
            int lines;
            void work(ccsds::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getImage(int channel);
        };
    } // namespace ceres
} // namespace aqua