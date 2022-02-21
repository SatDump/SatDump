#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "common/image/image.h"

namespace aqua
{
    namespace ceres
    {
        class CERESReader
        {
        private:
            unsigned short *channels[3];

        public:
            CERESReader();
            ~CERESReader();
            int lines;
            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getImage(int channel);
        };
    } // namespace ceres
} // namespace aqua