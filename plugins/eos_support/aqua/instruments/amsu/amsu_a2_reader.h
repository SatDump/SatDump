#pragma once

#include "common/ccsds/ccsds.h"
#include "image/image.h"

namespace aqua
{
    namespace amsu
    {
        class AMSUA2Reader
        {
        private:
            std::vector<uint16_t> channels[2];
            uint16_t lineBuffer[1000];

        public:
            AMSUA2Reader();
            ~AMSUA2Reader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            image::Image getChannel(int channel);
        };
    } // namespace amsu
} // namespace aqua