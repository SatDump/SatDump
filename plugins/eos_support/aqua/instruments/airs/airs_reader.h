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
            uint8_t shifted_buffer[7000];
            uint16_t line_buffer[4003 + 100];
            std::vector<uint16_t> channels[2666];
            std::vector<uint16_t> hd_channels[4];

        public:
            AIRSReader();
            ~AIRSReader();
            int lines;
            std::vector<std::vector<double>> timestamps_ifov;
            void work(ccsds::CCSDSPacket &packet);
            image::Image getChannel(int channel);
            image::Image getHDChannel(int channel);
        };
    } // namespace airs
} // namespace aqua