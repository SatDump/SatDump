#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"

namespace aqua
{
    namespace amsu
    {
        class AMSUA2Reader
        {
        private:
            unsigned short *channels[2];
            uint16_t lineBuffer[12944];

        public:
            AMSUA2Reader();
            ~AMSUA2Reader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace amsu
} // namespace aqua