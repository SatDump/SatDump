#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "common/image2/image.h"

namespace aqua
{
    namespace ceres
    {
        class CERESReader
        {
        private:
            std::vector<uint16_t> channels[3];

        public:
            CERESReader();
            ~CERESReader();
            int lines;
            void work(ccsds::CCSDSPacket &packet);
            image2::Image getImage(int channel);
        };
    } // namespace ceres
} // namespace aqua