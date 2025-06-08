#pragma once

#include "common/ccsds/ccsds.h"
#include <vector>
#include "image/image.h"

namespace earthcare
{
    namespace atlid
    {
        class ATLIDReader
        {
        private:
            std::vector<uint16_t> channel;

        public:
            ATLIDReader();
            ~ATLIDReader();

            void work(ccsds::CCSDSPacket &packet);
            image::Image getChannel();

            int lines = 0;
            std::vector<double> timestamps;
        };
    }
}