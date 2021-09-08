#pragma once

#include "common/ccsds/ccsds.h"
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include <map>
#include <array>

namespace fengyun
{
    namespace mwts2
    {
        class MWTS2Reader
        {
        private:
            std::map<double, std::array<std::array<unsigned short, 90>, 18>> imageData;
            double lastTime;
            unsigned short lineBuf[1000];

        public:
            MWTS2Reader();
            ~MWTS2Reader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    }
}
