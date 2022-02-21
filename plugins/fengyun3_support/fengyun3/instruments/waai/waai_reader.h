#pragma once

#include <cstdint>
#include "common/image/image.h"
#include <vector>
#include "common/resizeable_buffer.h"

namespace fengyun3
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
            image::Image<uint16_t> getChannel();
        };
    } // namespace virr
} // namespace fengyun