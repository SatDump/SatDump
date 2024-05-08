#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "common/image2/image.h"

namespace aura
{
    namespace omi
    {
        class OMIReader
        {
        private:
            uint16_t frameBuffer[2047 * 28];
            std::vector<uint16_t> channelRaw;
            std::vector<uint16_t> visibleChannel;
            std::vector<uint16_t> channels[792];

        public:
            OMIReader();
            ~OMIReader();
            int lines;
            void work(ccsds::CCSDSPacket &packet);
            image2::Image getChannel(int channel);
            image2::Image getImageRaw();
            image2::Image getImageVisible();
        };
    } // namespace ceres
} // namespace aqua