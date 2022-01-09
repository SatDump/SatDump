#pragma once

#include <cstdint>
#include "common/image/image.h"

namespace coriolis
{
    namespace windsat
    {
        class WindSatReader
        {
        private:
            unsigned short *image1, *image2;
            int channel_id;
            uint32_t scanline_id;
            int width;

        public:
            WindSatReader(int chid, int width);
            ~WindSatReader();
            int lines;
            void work(uint8_t *buffer);
            image::Image<uint16_t> getImage1();
            image::Image<uint16_t> getImage2();
        };
    } // namespace avhrr
} // namespace noaa