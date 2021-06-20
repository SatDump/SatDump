#pragma once

#include "common/ccsds/ccsds_1_0_1024/ccsds.h"
#include <map>
#include <array>

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace fengyun
{
    namespace erm
    {
        class ERMReader
        {
        private:
            std::map<time_t, std::array<unsigned short, 151>> imageData;

        public:
            ERMReader();
            ~ERMReader();
            int lines;
            void work(ccsds::ccsds_1_0_1024::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getChannel();
        };
    }
}