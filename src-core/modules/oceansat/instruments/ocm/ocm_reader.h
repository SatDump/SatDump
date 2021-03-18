#pragma once

#include <cstdint>

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace oceansat
{
    namespace ocm
    {
        class OCMReader
        {
        private:
            unsigned short *channels[6];

        public:
            OCMReader();
            ~OCMReader();
            int lines;
            void work(uint8_t *buffer);
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    } // namespace avhrr
} // namespace noaa