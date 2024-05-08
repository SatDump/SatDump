#pragma once

#include "common/ccsds/ccsds.h"
#include <map>
#include <array>
#include "common/image2/image.h"

namespace fengyun3
{
    namespace mwhs
    {
        class MWHSReader
        {
        private:
            std::map<double, std::array<std::array<unsigned short, 98>, 6>> imageData;
            double lastTime;
            unsigned short lineBuf[1000];

        public:
            MWHSReader();
            ~MWHSReader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            image2::Image getChannel(int channel);
        };
    }
}