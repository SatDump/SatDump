#pragma once

#include "common/ccsds/ccsds.h"
#include <vector>
#include "common/image/image.h"

namespace gcom1
{
    namespace amsr2
    {
        class AMSR2Reader
        {
        private:
            std::vector<uint16_t> channels[20];
            int current_pkt;

        public:
            AMSR2Reader();
            ~AMSR2Reader();

            void work(ccsds::CCSDSPacket &packet);

            int lines;

            image::Image<uint16_t> getChannel(int c);
        };
    } // namespace swap
} // namespace proba