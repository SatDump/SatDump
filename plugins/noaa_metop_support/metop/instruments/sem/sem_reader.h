#pragma once

#include "common/ccsds/ccsds.h"

namespace metop
{
    namespace sem
    {
        class SEMReader
        {
        public:
            int samples;
            std::vector<int> channels[40];
            std::vector<double> timestamps;

        public:
            SEMReader();
            ~SEMReader();
            void work(ccsds::CCSDSPacket &packet);
        };
    } // namespace modis
} // namespace eos