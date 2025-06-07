#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include <map>
// #include "image/image.h"
#include "libs/predict/predict.h"

namespace jason3
{
    namespace lpt
    {
        class LPTReader
        {
        private:
            const int start_byte;
            const int channel_count;
            const int pkt_size;

        public:
            LPTReader(int start_byte, int channel_count, int pkt_size);
            ~LPTReader();

            int frames;
            std::vector<std::vector<int>> channel_counts;
            std::vector<double> timestamps;

            void work(ccsds::CCSDSPacket &packet);
        };
    } // namespace modis
} // namespace eos