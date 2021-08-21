#pragma once

#include <cstdint>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace goes
{
    namespace gvar
    {
        class InfraredReader2
        {
        public:
            unsigned short *imageBuffer1;
            unsigned short *imageBuffer2;
            unsigned short *imageLineBuffer;

        private:
            bool *goodLines;

        public:
            int frames;
            InfraredReader2();
            ~InfraredReader2();
            void startNewFullDisk();
            void pushFrame(uint8_t *data, int counter, int word_cnt);
            cimg_library::CImg<unsigned short> getImage1();
            cimg_library::CImg<unsigned short> getImage2();
        };
    }
}