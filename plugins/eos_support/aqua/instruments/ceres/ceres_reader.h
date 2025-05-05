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
            std::vector<uint16_t> channels[3];

        public:
            CERESReader();
            ~CERESReader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            image::Image getImage(int channel);
        };
    } // namespace ceres
} // namespace aqua