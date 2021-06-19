#pragma once

#include <cstdint>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include <vector>
#include "common/resizeable_buffer.h"

namespace fengyun
{
    namespace waai
    {
        class WAAIReader
        {
        private:
            unsigned short *imageBuffer;

        public:
            WAAIReader();
            ~WAAIReader();
            int lines;
            void work(std::vector<uint8_t> &packet);
            cimg_library::CImg<unsigned short> getChannel();
        };
    } // namespace virr
} // namespace fengyun