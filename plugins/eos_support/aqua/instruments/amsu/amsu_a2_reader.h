#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image2/image.h"

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
            image2::Image getChannel(int channel);
        };
    } // namespace amsu
} // namespace aqua