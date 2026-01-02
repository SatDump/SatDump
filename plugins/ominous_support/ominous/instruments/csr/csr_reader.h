#pragma once

#include "common/ccsds/ccsds.h"
#include "image/image.h"
#include <vector>

namespace ominous
{
    namespace csr
    {
        class CSRReader
        {
        private:
            std::vector<uint16_t> channels[10];

        public:
            CSRReader();
            ~CSRReader();

            void work(ccsds::CCSDSPacket &packet);
            image::Image getChannel(int channel);

            int lines = 0;
            std::vector<double> timestamps;
        };
    } // namespace csr
} // namespace ominous