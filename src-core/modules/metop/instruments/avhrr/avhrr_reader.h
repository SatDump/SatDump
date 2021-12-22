#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"
#include "common/resizeable_buffer.h"

namespace metop
{
    namespace avhrr
    {
        class AVHRRReader
        {
        private:
            ResizeableBuffer<unsigned short> channels[5];
            uint16_t avhrr_buffer[10355];

        public:
            AVHRRReader();
            ~AVHRRReader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace avhrr
} // namespace metop