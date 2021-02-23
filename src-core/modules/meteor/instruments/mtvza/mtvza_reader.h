#pragma once

#include <cstdint>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace meteor
{
    namespace mtvza
    {
        class MTVZAReader
        {
        private:
            unsigned short *channels[150];

        public:
            MTVZAReader();
            ~MTVZAReader();
            int lines;
            void work(uint8_t *data);
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    }
}