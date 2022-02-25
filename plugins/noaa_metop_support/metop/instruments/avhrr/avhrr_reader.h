#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"

namespace metop
{
    namespace avhrr
    {
        class AVHRRReader
        {
        private:
            uint16_t avhrr_buffer[10355];

        public:
            int lines;
            std::vector<uint16_t> channels[5];
            std::vector<double> timestamps;

        public:
            AVHRRReader();
            ~AVHRRReader();
            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace avhrr
} // namespace metop