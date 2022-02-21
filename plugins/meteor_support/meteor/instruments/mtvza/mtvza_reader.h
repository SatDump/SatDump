#pragma once

#include <cstdint>
#include "common/image/image.h"

namespace meteor
{
    namespace mtvza
    {
        class MTVZAReader
        {
        private:
            unsigned short *channels[60];

        public:
            MTVZAReader();
            ~MTVZAReader();
            int lines;
            void work(uint8_t *data);
            image::Image<uint16_t> getChannel(int channel);
        };
    }
}