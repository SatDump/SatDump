#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"

namespace metop
{
    namespace ascat
    {
        class ASCATReader
        {
        private:
            unsigned short *channels[6];

        public:
            ASCATReader();
            ~ASCATReader();
            int lines[6];
            std::vector<double> timestamps[6];
            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace avhrr
} // namespace metop