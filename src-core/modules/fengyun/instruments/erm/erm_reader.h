#pragma once

#include "common/ccsds/ccsds_1_0_1024/ccsds.h"
#include <vector>

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace fengyun
{
    namespace erm
    {
        class ERMImage
        {
        public:
            unsigned short imageData[151 * 32];
            int mk = -1;
            int lastMkMatch;
        };

        class ERMReader
        {
        private:
            std::vector<ERMImage> imageVector;

        public:
            ERMReader();
            ~ERMReader();
            int lines;
            void work(ccsds::ccsds_1_0_1024::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getChannel();
        };
    }
}