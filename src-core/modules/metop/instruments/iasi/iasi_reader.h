#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"

namespace metop
{
    namespace iasi
    {
        class IASIReader
        {
        private:
            unsigned short *channels[8461];

        public:
            IASIReader();
            ~IASIReader();
            int lines;
            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace iasi
} // namespace metop