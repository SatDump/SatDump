#pragma once

#include <cstdint>
#include "common/image/image.h"

namespace noaa
{
    namespace avhrr
    {
        class AVHRRReader
        {
        private:
            unsigned short *channels[5];

        public:
            AVHRRReader();
            ~AVHRRReader();
            int lines;
            void work(uint16_t *buffer);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace avhrr
} // namespace noaa