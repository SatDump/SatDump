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
            int scan_pos;
            std::vector<uint16_t> channels[22];

        public:
            ATMSReader();
            ~ATMSReader();

            int lines;
            std::vector<double> timestamps;

            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace atms
} // namespace jpss