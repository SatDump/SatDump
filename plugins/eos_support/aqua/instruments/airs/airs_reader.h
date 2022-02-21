#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"

namespace aqua
{
    namespace airs
    {
        class AIRSReader
        {
        private:
            unsigned short *channels[2666];
            unsigned short *hd_channels[4];

        public:
            AIRSReader();
            ~AIRSReader();
            int lines;
            std::vector<std::vector<double>> timestamps_ifov;
            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getChannel(int channel);
            image::Image<uint16_t> getHDChannel(int channel);
        };
    } // namespace airs
} // namespace aqua