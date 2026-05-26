#pragma once

#include "common/ccsds/ccsds.h"
#include "logger.h"
#include <cstdint>
#include <vector>

namespace ssdv
{
    namespace ssdvng
    {
        class SSDVNGReader
        {
        private:
            std::vector<uint8_t> full_pkt;

        public:
            SSDVNGReader();
            ~SSDVNGReader();
            int img_cnt = 0;
            void work(ccsds::CCSDSPacket &packet);
        };
    } // namespace ssdvng
} // namespace ssdv
