#pragma once

#include "common/ccsds/ccsds.h"
#include <map>
#include <array>
#include "common/image/image.h"

namespace fengyun3
{
    namespace mwhs2
    {
        class MWHS2Reader
        {
        private:
            std::map<double, std::array<std::array<unsigned short, 98>, 15>> imageData;
            double lastTime;
            unsigned short lineBuf[1000];

        public:
            MWHS2Reader();
            ~MWHS2Reader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet, bool fy3d_mode);
            image::Image<uint16_t> getChannel(int channel);
        };
    }
}