#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"
#include <map>
#include <array>

namespace fengyun3
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
            image::Image<uint16_t> getChannel(int channel);
        };
    }
}
