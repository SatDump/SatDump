#pragma once

#include <cstdint>
#include "common/image/image.h"

namespace oceansat
{
    namespace ocm
    {
        class OCMReader
        {
        private:
            unsigned short *channels[8];
            unsigned short *lineBuffer;

        public:
            OCMReader();
            ~OCMReader();
            int lines;
            void work(uint8_t *buffer);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace avhrr
} // namespace noaa