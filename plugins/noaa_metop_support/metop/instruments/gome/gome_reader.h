#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"

namespace metop
{
    namespace gome
    {
        class GOMEReader
        {
        public:
            int lines;
            std::vector<uint16_t> channels[6][1024];

            // Values extracted from the packet according to the "rough" documenation in the Metopizer
            int band_channels[6] = {0, 0, 1, 1, 2, 3};
            int band_starts[6] = {0, 659, 0, 71, 0, 0};
            int band_ends[6] = {658, 1023, 70, 1023, 1023, 1023};

            int channel_number = 0;

        public:
            GOMEReader();
            ~GOMEReader();
            void work(ccsds::CCSDSPacket &packet);
            std::vector<double> timestamps;
            image::Image getChannel(int channel);
        };
    } // namespace gome
} // namespace metop