#pragma once

#include <cstdint>
#include "image/image.h"
#include <vector>

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
            image::Image getChannel();
        };
    } // namespace virr
} // namespace fengyun