#pragma once

#include <cstdint>
#include <vector>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace fengyun
{
    namespace mersi2
    {
        class MERSI250Reader
        {
        private:
            unsigned short *imageBuffer;
            unsigned short *mersiLineBuffer;
            int frames;
            uint8_t byteBufShift[3];

        public:
            MERSI250Reader();
            ~MERSI250Reader();
            void pushFrame(std::vector<uint8_t> &data);
            cimg_library::CImg<unsigned short> getImage();
        };
    } // namespace mersi2
} // namespace fengyun