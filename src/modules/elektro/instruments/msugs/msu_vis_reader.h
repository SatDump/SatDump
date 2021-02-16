#pragma once

#include <cstdint>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace elektro
{
    namespace msugs
    {
        class MSUVISReader
        {
        private:
            unsigned short *imageBuffer;
            unsigned short msuLineBuffer[12044];
            int frames;

        public:
            MSUVISReader();
            ~MSUVISReader();
            void pushFrame(uint8_t *data);
            cimg_library::CImg<unsigned short> getImage();
        };
    } // namespace msugs
} // namespace elektro