#pragma once

#include <cstdint>
#include "common/image/image.h"

namespace cloudsat
{
    namespace cpr
    {
        class CPReader
        {
        private:
            unsigned short *image;
            uint32_t buffer20[2000];

        public:
            CPReader();
            ~CPReader();
            int lines;
            void work(uint8_t *buffer);
            image::Image getChannel();
        };
    } // namespace avhrr
} // namespace noaa