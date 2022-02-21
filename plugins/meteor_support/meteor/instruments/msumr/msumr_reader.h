#pragma once

#include <cstdint>
#include "common/image/image.h"

namespace meteor
{
    namespace msumr
    {
        class MSUMRReader
        {
        private:
            unsigned short *channels[6];
            uint16_t msumrBuffer[6][1572];

        public:
            MSUMRReader();
            ~MSUMRReader();
            int lines;
            void work(uint8_t *buffer);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace msumr
} // namespace meteor