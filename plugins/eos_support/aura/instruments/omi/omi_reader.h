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
            unsigned short *frameBuffer;
            unsigned short *channelRaw;
            unsigned short *visibleChannel;
            unsigned short *channels[792];

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