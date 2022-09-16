#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"

namespace metop
{
    namespace amsu
    {
        class AMSUA2Reader
        {
        private:
            uint16_t lineBuffer[12944];

        public:
            std::vector<uint16_t> channels[2];
            int lines;
            std::vector<double> timestamps;

        public:
            AMSUA2Reader();
            ~AMSUA2Reader();
            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace amsu
} // namespace metop