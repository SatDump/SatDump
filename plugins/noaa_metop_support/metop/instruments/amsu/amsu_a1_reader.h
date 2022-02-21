#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"

namespace metop
{
    namespace amsu
    {
        class AMSUA1Reader
        {
        private:
            unsigned short *channels[13];
            uint16_t lineBuffer[12944];

        public:
            AMSUA1Reader();
            ~AMSUA1Reader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace amsu
} // namespace metop