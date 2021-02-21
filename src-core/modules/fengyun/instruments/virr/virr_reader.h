#pragma once

#include <cstdint>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include <vector>

namespace fengyun
{
    namespace virr
    {
        class VIRRReader
        {
        private:
            unsigned short *channels[10];
            uint16_t virrBuffer[204800];

        public:
            VIRRReader();
            int lines;
            void work(std::vector<uint8_t> &packet);
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    } // namespace virr
} // namespace fengyun