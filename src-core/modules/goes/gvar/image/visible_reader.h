#pragma once

#include <cstdint>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace goes
{
    namespace gvar
    {
        class VisibleReader
        {
        public:
            unsigned short *imageBuffer;
            unsigned short *imageLineBuffer;

        private:
            uint8_t byteBufShift[5];
            bool *goodLines;

        public:
            int frames;
            VisibleReader();
            ~VisibleReader();
            void startNewFullDisk();
            void pushFrame(uint8_t *data, int block, int counter);
            cimg_library::CImg<unsigned short> getImage();
        };
    }
}