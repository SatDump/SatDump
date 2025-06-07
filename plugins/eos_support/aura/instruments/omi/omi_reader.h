#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "image/image.h"

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
            image::Image getChannel(int channel);
            image::Image getImageRaw();
            image::Image getImageVisible();
        };
    } // namespace ceres
} // namespace aqua