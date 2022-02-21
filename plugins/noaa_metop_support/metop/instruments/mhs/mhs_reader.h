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
            unsigned short *channels[5];
            uint16_t mhs_buffer[540];

        public:
            MHSReader();
            ~MHSReader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace mhs
} // namespace metop