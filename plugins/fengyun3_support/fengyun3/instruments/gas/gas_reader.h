#pragma once

#include <cstdint>
#include "common/image2/image.h"
#include <vector>
#include "common/resizeable_buffer.h"

namespace fengyun3
{
    namespace gas
    {
        class GASReader
        {
        private:
            unsigned short *imageBuffer;

        public:
            GASReader();
            ~GASReader();
            int lines;
            void work(std::vector<uint8_t> &packet);
            image2::Image getChannel();
        };
    } // namespace virr
} // namespace fengyun