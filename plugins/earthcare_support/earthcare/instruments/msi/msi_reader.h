#pragma once

#include "common/ccsds/ccsds.h"
#include <vector>
#include "common/image/image.h"

namespace earthcare
{
    namespace msi
    {
        class MSIReader
        {
        private:
            std::vector<uint16_t> channels[7];

        public:
            MSIReader();
            ~MSIReader();

            void work(ccsds::CCSDSPacket &packet);
            image::Image getChannel(int channel);

            int lines = 0;
            std::vector<double> timestamps;
        };
    }
}