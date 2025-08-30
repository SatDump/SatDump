#pragma once

#include "common/ccsds/ccsds.h"
#include "image/image.h"
#include "nlohmann/json.hpp"

namespace metopsg
{
    namespace mws
    {
        class MWSReader
        {
        public:
            int lines[24];
            std::vector<uint16_t> channels[24];
            std::vector<double> timestamps;

        public:
            MWSReader();
            ~MWSReader();
            void work(ccsds::CCSDSPacket &packet);

            image::Image getChannel(int c);
        };
    } // namespace mws
} // namespace metopsg