#pragma once

#include "common/ccsds/ccsds_1_0_1024/ccsds.h"
#include <cmath>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace aura
{
    namespace omi
    {
        class OMIReader
        {
        private:
            unsigned short *channels[3];

        public:
            OMIReader();
            ~OMIReader();
            int lines;
            void work(ccsds::ccsds_1_0_1024::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getImage(int channel);
        };
    } // namespace ceres
} // namespace aqua