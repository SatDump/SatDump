#pragma once

#include <cstdint>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace goes
{
    namespace gvar
    {
        class InfraredReader1
        {
        public:
            unsigned short *imageBuffer1;
            unsigned short *imageBuffer2;
            unsigned short *imageLineBuffer;

        private:
            uint8_t byteBufShift[5];
            bool *goodLines;

        public:
            int frames;
            InfraredReader1();
            ~InfraredReader1();
            void startNewFullDisk();
            void pushFrame(uint8_t *data, int counter);
            cimg_library::CImg<unsigned short> getImage1();
            cimg_library::CImg<unsigned short> getImage2();
        };
    }
}