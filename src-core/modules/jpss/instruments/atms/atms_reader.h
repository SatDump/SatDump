#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "common/image/image.h"

namespace jpss
{
    namespace atms
    {
        class ATMSReader
        {
        private:
            int endSequenceCount;
            unsigned short *channels[22];
            bool inScan;

        public:
            ATMSReader();
            ~ATMSReader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getImage(int channel);
        };
    } // namespace atms
} // namespace jpss