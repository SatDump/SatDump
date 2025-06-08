#pragma once

#include "common/ccsds/ccsds.h"
#include "image/image.h"

namespace metop
{
    namespace iasi
    {
        class IASIReader
        {
        private:
            std::vector<uint16_t> channels[8461];

        public:
            IASIReader();
            ~IASIReader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            image::Image getChannel(int channel);
        };
    } // namespace iasi
} // namespace metop