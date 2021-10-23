#pragma once

#include <cstdint>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include <vector>
#include "common/resizeable_buffer.h"

namespace fengyun3
{
    namespace mwri
    {
        class MWRIReader
        {
        private:
            ResizeableBuffer<unsigned short> channels[10];

        public:
            MWRIReader();
            ~MWRIReader();
            int lines;
            void work(std::vector<uint8_t> &packet);
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    } // namespace virr
} // namespace fengyun