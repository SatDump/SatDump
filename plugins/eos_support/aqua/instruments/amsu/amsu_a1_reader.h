#pragma once

#include "common/ccsds/ccsds.h"
#include "image/image.h"

namespace aqua
{
    namespace amsu
    {
        class AMSUA1Reader
        {
        private:
            std::vector<uint16_t> channels[13];
            uint16_t lineBuffer[1000];

        public:
            AMSUA1Reader();
            ~AMSUA1Reader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            image::Image getChannel(int channel);
        };
    } // namespace amsu
} // namespace aqua