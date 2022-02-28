#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "common/image/image.h"

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
            image::Image<uint16_t> getChannel(int channel);
            image::Image<uint16_t> getImageRaw();
            image::Image<uint16_t> getImageVisible();
        };
    } // namespace ceres
} // namespace aqua