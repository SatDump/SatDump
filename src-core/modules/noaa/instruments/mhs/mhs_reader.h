#pragma once

#include <cstdint>

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace noaa
{
    namespace mhs
    {
        class MHSReader
        {
        private:
            unsigned short *channels[5];
            unsigned int last;
            uint16_t MHSWord[643];
        public:
            MHSReader();
            ~MHSReader();
            int line = 0;
            void work(uint8_t *buffer);
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    } // namespace hirs
} // namespace noaa