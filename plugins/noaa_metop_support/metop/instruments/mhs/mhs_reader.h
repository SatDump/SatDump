#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"

namespace metop
{
    namespace mhs
    {
        class MHSReader
        {
        private:
            uint16_t mhs_buffer[540];

        public:
            int lines;
            std::vector<uint16_t> channels[5];
            std::vector<double> timestamps;

        public:
            MHSReader();
            ~MHSReader();
            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace mhs
} // namespace metop