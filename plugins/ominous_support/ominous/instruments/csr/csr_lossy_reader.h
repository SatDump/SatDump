#pragma once

#include "common/ccsds/ccsds.h"
#include "image/image.h"
#include <vector>

namespace ominous
{
    namespace csr
    {
        class CSRLossyReader
        {
        private:
            struct CSRLossySegment
            {
                double timestamp;
                int pos;
                image::Image img;
            };
            std::vector<CSRLossySegment> channels[10];

        public:
            CSRLossyReader();
            ~CSRLossyReader();

            void work(ccsds::CCSDSPacket &packet);
            image::Image getChannel(int channel);

            int segments = 0;
            std::vector<double> timestamps;
        };
    } // namespace csr
} // namespace ominous