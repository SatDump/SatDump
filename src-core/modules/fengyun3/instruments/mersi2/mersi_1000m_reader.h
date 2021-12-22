#pragma once

#include <cstdint>
#include <vector>
#include "common/image/image.h"
#include "common/resizeable_buffer.h"

namespace fengyun3
{
    namespace mersi2
    {
        class MERSI1000Reader
        {
        private:
            ResizeableBuffer<unsigned short> imageBuffer;
            unsigned short *mersiLineBuffer;
            int frames;
            uint8_t byteBufShift[3];

        public:
            MERSI1000Reader();
            ~MERSI1000Reader();
            void pushFrame(std::vector<uint8_t> &data);
            image::Image<uint16_t> getImage();
        };
    } // namespace mersi1
} // namespace fengyun