#pragma once

#include <cstdint>
#include <vector>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include "common/resizeable_buffer.h"

namespace fengyun3
{
    namespace mersi1
    {
        class MERSI250Reader
        {
        private:
            ResizeableBuffer<unsigned short> imageBuffer;
            unsigned short *mersiLineBuffer;
            int frames;
            uint8_t byteBufShift[3];

        public:
            MERSI250Reader();
            ~MERSI250Reader();
            void pushFrame(std::vector<uint8_t> &data);
            cimg_library::CImg<unsigned short> getImage();
        };
    } // namespace mersi1
} // namespace fengyun