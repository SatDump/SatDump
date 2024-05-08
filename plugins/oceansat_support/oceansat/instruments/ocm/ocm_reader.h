#pragma once

#include <cstdint>
#include "common/image2/image.h"

namespace oceansat
{
    namespace ocm
    {
        class OCMReader
        {
        private:
            std::vector<uint16_t> channels[8];
            uint16_t lineBuffer[4072 * 10];

        public:
            OCMReader();
            ~OCMReader();
            int lines;
            void work(uint8_t *buffer);
            image2::Image getChannel(int channel);
        };
    } // namespace avhrr
} // namespace noaa