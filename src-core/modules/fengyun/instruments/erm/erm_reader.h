#pragma once

#include "common/ccsds/ccsds.h"
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
            void work(ccsds::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getChannel();
        };
    }
}